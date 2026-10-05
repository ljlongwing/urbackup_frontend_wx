/*************************************************************************
*    UrBackup - Client/Server backup system
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "ServerListUI.h"
#include "stringtools.h"
#include "Settings.h"
#include "main.h"
#include "capa_bits.h"
#include <wx/datetime.h>

wxTextValidator getPathValidator(void);

namespace
{
	enum
	{
		COL_NAME = 0,
		COL_LOCAL,
		COL_INTERNET,
		COL_STATUS,
		COL_FINGERPRINT
	};

	wxString statusText(const SServerListEntry& entry)
	{
		wxString status;
		if (!entry.local && !entry.internet)
		{
			return _("Disabled");
		}
		if (entry.local
			&& entry.online
			&& !(entry.internet && entry.internet_status == "connected"))
		{
			status = _("Connected (LAN)");
		}
		if (entry.internet)
		{
			wxString internet_status;
			if (entry.internet_status == "connected")
				internet_status = _("Connected (internet)");
			else if (entry.internet_status == "connected_local")
				internet_status = _("Internet paused (LAN)");
			else if (entry.internet_status == "wait_local" || entry.internet_status == "initializing")
				internet_status = _("Waiting");
			else if (entry.internet_status == "connecting_failed")
				internet_status = _("Internet connection failed");
			else if (entry.internet_status.find("error:") == 0)
				internet_status = wxString::FromUTF8(entry.internet_status.substr(6).c_str());
			else if (!entry.internet_status.empty())
				internet_status = wxString::FromUTF8(entry.internet_status.c_str());

			if (!internet_status.empty())
			{
				if (!status.empty()) status += wxT(", ");
				status += internet_status;
			}
		}
		if (status.empty())
		{
			status = entry.ident.empty() ? _("Not connected yet") : _("Offline");
		}
		return status;
	}

	wxString fingerprintText(const SServerListEntry& entry)
	{
		if (!entry.fingerprint.empty())
		{
			//Short form, like an SSH key fingerprint
			std::string fp = entry.fingerprint;
			if (fp.size() > 23)
				fp = fp.substr(0, 23) + "...";
			return wxString::FromUTF8(fp.c_str());
		}
		if (!entry.ident.empty())
		{
			return wxString::FromUTF8(entry.ident.c_str());
		}
		return wxT("-");
	}
}

wxString ServersPanel::displayName(const SServerListEntry& entry)
{
	if (!entry.name.empty())
		return wxString::FromUTF8(entry.name.c_str());
	if (!entry.endpoint.empty())
		return wxString::FromUTF8(entry.endpoint.c_str());
	if (!entry.internet_server.empty())
		return wxString::FromUTF8(entry.internet_server.c_str());
	return _("(unnamed server)");
}

wxString ServersPanel::toUrl(const SServerListEntry& entry)
{
	wxString server = wxString::FromUTF8(entry.internet_server.c_str());
	if (server.empty()
		|| server.find(wxT("urbackup://")) == 0
		|| server.find(wxT("ws://")) == 0
		|| server.find(wxT("wss://")) == 0)
	{
		return server;
	}

	wxString url = wxT("urbackup://") + server;
	if (!entry.internet_server_port.empty()
		&& entry.internet_server_port != "55415")
	{
		url += wxT(":") + wxString::FromUTF8(entry.internet_server_port.c_str());
	}
	return url;
}

void ServersPanel::fromUrl(wxString url, SServerListEntry& entry)
{
	url.Trim().Trim(false);
	entry.internet_server_port = "55415";

	if (!url.empty()
		&& url.find(wxT("urbackup://")) != 0
		&& url.find(wxT("ws://")) != 0
		&& url.find(wxT("wss://")) != 0)
	{
		url = wxT("urbackup://") + url;
	}

	if (url.find(wxT("urbackup://")) == 0)
	{
		wxString hostname = url.substr(11);
		if (hostname.find(wxT(":")) != wxString::npos)
		{
			entry.internet_server_port = std::string(hostname.AfterFirst(':').ToUTF8());
			entry.internet_server = std::string(hostname.BeforeFirst(':').ToUTF8());
		}
		else
		{
			entry.internet_server = std::string(hostname.ToUTF8());
		}
	}
	else
	{
		entry.internet_server = std::string(url.ToUTF8());
	}
}

ServerEditDialog::ServerEditDialog(wxWindow* parent, const SServerListEntry& p_entry)
	: wxDialog(parent, wxID_ANY, p_entry.id < 0 ? _("Add server") : _("Edit server"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	entry(p_entry)
{
	wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* grid = new wxFlexGridSizer(2, wxDLG_UNIT(this, wxSize(4, 3)));
	grid->AddGrowableCol(1);

	wxSize input_size = wxDLG_UNIT(this, wxSize(150, -1));

	grid->Add(new wxStaticText(this, wxID_ANY, _("Name:")), 0, wxALIGN_CENTER_VERTICAL);
	m_name = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(entry.name.c_str()), wxDefaultPosition, input_size);
	m_name->SetHint(ServersPanel::displayName(entry));
	grid->Add(m_name, 1, wxEXPAND);

	if (!entry.ident.empty())
	{
		grid->Add(new wxStaticText(this, wxID_ANY, _("Server identity:")), 0, wxALIGN_CENTER_VERTICAL);
		wxString ident = wxString::FromUTF8(entry.fingerprint.empty() ? entry.ident.c_str() : entry.fingerprint.c_str());
		wxTextCtrl* ident_ctrl = new wxTextCtrl(this, wxID_ANY, ident, wxDefaultPosition, input_size, wxTE_READONLY);
		grid->Add(ident_ctrl, 1, wxEXPAND);
	}

	grid->Add(new wxStaticText(this, wxID_ANY, wxEmptyString));
	m_local = new wxCheckBox(this, wxID_ANY, _("Local (LAN): accept backups from this server in the local network"));
	m_local->SetValue(entry.local);
	grid->Add(m_local);

	grid->Add(new wxStaticText(this, wxID_ANY, wxEmptyString));
	m_internet = new wxCheckBox(this, wxID_ANY, _("Internet: connect to this server via internet"));
	m_internet->SetValue(entry.internet);
	grid->Add(m_internet);

	grid->Add(new wxStaticText(this, wxID_ANY, _("Server URL:")), 0, wxALIGN_CENTER_VERTICAL);
	m_url = new wxTextCtrl(this, wxID_ANY, ServersPanel::toUrl(entry), wxDefaultPosition, input_size);
	m_url->SetHint(wxT("urbackup://backup.example.com"));
	grid->Add(m_url, 1, wxEXPAND);

	grid->Add(new wxStaticText(this, wxID_ANY, _("HTTP(s) proxy:")), 0, wxALIGN_CENTER_VERTICAL);
	m_proxy = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(entry.internet_server_proxy.c_str()), wxDefaultPosition, input_size);
	grid->Add(m_proxy, 1, wxEXPAND);

	grid->Add(new wxStaticText(this, wxID_ANY, _("Server password:")), 0, wxALIGN_CENTER_VERTICAL);
	m_authkey = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(entry.internet_authkey.c_str()), wxDefaultPosition, input_size, wxTE_PASSWORD);
	grid->Add(m_authkey, 1, wxEXPAND);

	grid->Add(new wxStaticText(this, wxID_ANY, wxEmptyString));
	m_compress = new wxCheckBox(this, wxID_ANY, _("Compressed transfer"));
	m_compress->SetValue(entry.internet_compress);
	grid->Add(m_compress);

	grid->Add(new wxStaticText(this, wxID_ANY, wxEmptyString));
	m_encrypt = new wxCheckBox(this, wxID_ANY, _("Encrypted transfer"));
	m_encrypt->SetValue(entry.internet_encrypt);
	grid->Add(m_encrypt);

	top->Add(grid, 1, wxEXPAND | wxALL, 10);
	top->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
	SetSizerAndFit(top);

	m_internet->Bind(wxEVT_CHECKBOX, &ServerEditDialog::OnInternetCheck, this);
	Bind(wxEVT_BUTTON, &ServerEditDialog::OnOk, this, wxID_OK);

	updateEnabled();
	CentreOnParent();
}

void ServerEditDialog::updateEnabled()
{
	bool internet = m_internet->GetValue();
	m_url->Enable(internet);
	m_proxy->Enable(internet);
	m_authkey->Enable(internet);
	m_compress->Enable(internet);
	m_encrypt->Enable(internet);
}

void ServerEditDialog::OnInternetCheck(wxCommandEvent& event)
{
	updateEnabled();
}

void ServerEditDialog::OnOk(wxCommandEvent& event)
{
	if (!m_local->GetValue() && !m_internet->GetValue())
	{
		wxMessageBox(_("Select at least one of \"Local (LAN)\" and \"Internet\". "
			"To stop using a server, remove it from the list."), _("Server"), wxOK | wxICON_WARNING, this);
		return;
	}

	if (m_internet->GetValue()
		&& (m_url->GetValue().Trim().empty() || m_authkey->GetValue().empty()))
	{
		wxMessageBox(_("Internet connections need the server URL and the server password."),
			_("Server"), wxOK | wxICON_WARNING, this);
		return;
	}

	entry.name = std::string(m_name->GetValue().Trim().Trim(false).ToUTF8());
	entry.local = m_local->GetValue();
	entry.internet = m_internet->GetValue();
	ServersPanel::fromUrl(m_url->GetValue(), entry);
	entry.internet_server_proxy = std::string(m_proxy->GetValue().Trim().Trim(false).ToUTF8());
	entry.internet_authkey = std::string(m_authkey->GetValue().ToUTF8());
	entry.internet_compress = m_compress->GetValue();
	entry.internet_encrypt = m_encrypt->GetValue();

	EndModal(wxID_OK);
}

ServersPanel::ServersPanel(wxWindow* parent, const SServerList& server_list)
	: wxPanel(parent, wxID_ANY), entries(server_list.entries), pending(server_list.pending), modified(false)
{
	wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);

	top->Add(new wxStaticText(this, wxID_ANY,
		_("Servers this client is backed up by. Each server can be reached in the local network, via internet, or both.")),
		0, wxALL, 5);

	m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(330, 110)),
		wxLC_REPORT | wxLC_SINGLE_SEL);
	m_list->InsertColumn(COL_NAME, _("Name"), wxLIST_FORMAT_LEFT, wxDLG_UNIT(this, wxSize(80, -1)).GetWidth());
	m_list->InsertColumn(COL_LOCAL, _("Local"), wxLIST_FORMAT_LEFT, wxDLG_UNIT(this, wxSize(28, -1)).GetWidth());
	m_list->InsertColumn(COL_INTERNET, _("Internet"), wxLIST_FORMAT_LEFT, wxDLG_UNIT(this, wxSize(35, -1)).GetWidth());
	m_list->InsertColumn(COL_STATUS, _("Status"), wxLIST_FORMAT_LEFT, wxDLG_UNIT(this, wxSize(90, -1)).GetWidth());
	m_list->InsertColumn(COL_FINGERPRINT, _("Identity"), wxLIST_FORMAT_LEFT, wxDLG_UNIT(this, wxSize(90, -1)).GetWidth());
	top->Add(m_list, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
	m_add = new wxButton(this, wxID_ANY, _("Add..."));
	m_edit = new wxButton(this, wxID_ANY, _("Edit..."));
	m_remove = new wxButton(this, wxID_ANY, _("Remove"));
	m_trust = new wxButton(this, wxID_ANY, _("Trust new server..."));
	buttons->Add(m_add, 0, wxRIGHT, 5);
	buttons->Add(m_edit, 0, wxRIGHT, 5);
	buttons->Add(m_remove, 0, wxRIGHT, 5);
	buttons->AddStretchSpacer();
	buttons->Add(m_trust, 0);
	top->Add(buttons, 0, wxEXPAND | wxALL, 5);

	SetSizerAndFit(top);

	m_add->Bind(wxEVT_BUTTON, &ServersPanel::OnAdd, this);
	m_edit->Bind(wxEVT_BUTTON, &ServersPanel::OnEdit, this);
	m_remove->Bind(wxEVT_BUTTON, &ServersPanel::OnRemove, this);
	m_trust->Bind(wxEVT_BUTTON, &ServersPanel::OnTrust, this);
	m_list->Bind(wxEVT_LIST_ITEM_SELECTED, &ServersPanel::OnSelect, this);
	m_list->Bind(wxEVT_LIST_ITEM_DESELECTED, &ServersPanel::OnSelect, this);
	m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ServersPanel::OnActivate, this);

	fillList();
}

void ServersPanel::fillList()
{
	m_list->DeleteAllItems();
	for (size_t i = 0; i < entries.size(); ++i)
	{
		const SServerListEntry& e = entries[i];
		long item = m_list->InsertItem(static_cast<long>(i), displayName(e));
		m_list->SetItem(item, COL_LOCAL, e.local ? _("Yes") : _("No"));
		m_list->SetItem(item, COL_INTERNET, e.internet ? _("Yes") : _("No"));
		m_list->SetItem(item, COL_STATUS, statusText(e));
		m_list->SetItem(item, COL_FINGERPRINT, fingerprintText(e));
	}
	updateButtons();
}

long ServersPanel::selectedItem()
{
	return m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

void ServersPanel::updateButtons()
{
	bool has_selection = selectedItem() >= 0;
	m_edit->Enable(has_selection);
	m_remove->Enable(has_selection);
	m_trust->Enable(!pending.empty());
}

void ServersPanel::OnSelect(wxListEvent& event)
{
	updateButtons();
}

void ServersPanel::OnActivate(wxListEvent& event)
{
	wxCommandEvent evt;
	OnEdit(evt);
}

void ServersPanel::OnAdd(wxCommandEvent& event)
{
	SServerListEntry entry;
	entry.id = -1;
	entry.local = false;
	entry.internet = true;
	ServerEditDialog dlg(this, entry);
	if (dlg.ShowModal() == wxID_OK)
	{
		entries.push_back(dlg.getEntry());
		modified = true;
		fillList();
	}
}

void ServersPanel::OnEdit(wxCommandEvent& event)
{
	long item = selectedItem();
	if (item < 0 || item >= static_cast<long>(entries.size()))
		return;

	ServerEditDialog dlg(this, entries[item]);
	if (dlg.ShowModal() == wxID_OK)
	{
		entries[item] = dlg.getEntry();
		modified = true;
		fillList();
	}
}

void ServersPanel::OnRemove(wxCommandEvent& event)
{
	long item = selectedItem();
	if (item < 0 || item >= static_cast<long>(entries.size()))
		return;

	wxString msg = wxString::Format(_("Remove server \"%s\"? The client will no longer trust it, "
		"so it cannot back up this computer anymore until it is added again."), displayName(entries[item]));
	if (wxMessageBox(msg, _("Remove server"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
		return;

	entries.erase(entries.begin() + item);
	modified = true;
	fillList();
}

void ServersPanel::OnTrust(wxCommandEvent& event)
{
	if (pending.empty())
		return;

	wxArrayString choices;
	for (size_t i = 0; i < pending.size(); ++i)
	{
		choices.Add(wxString::FromUTF8(pending[i].c_str()));
	}

	int idx = wxGetSingleChoiceIndex(_("These servers tried to back up this computer, but are not trusted. "
		"Only trust a server you know."), _("Trust new server"), choices, this);
	if (idx < 0)
		return;

	if (!Connector::addNewServer(pending[idx]))
	{
		wxMessageBox(_("Trusting the server failed."), _("Trust new server"), wxOK | wxICON_ERROR, this);
		return;
	}

	SServerList server_list = Connector::getServerList();
	if (server_list.supported)
	{
		//Keep local edits, add the newly trusted server
		for (size_t i = 0; i < server_list.entries.size(); ++i)
		{
			bool found = false;
			for (size_t j = 0; j < entries.size(); ++j)
			{
				if (entries[j].id == server_list.entries[i].id)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				entries.push_back(server_list.entries[i]);
			}
		}
		pending = server_list.pending;
	}
	fillList();
}

namespace
{
	enum
	{
		//Buttons of a server row: ID_SERVER_BUTTON + 3*index + action
		ID_SERVER_BUTTON = wxID_HIGHEST + 100,
		SERVER_ACTION_SETTINGS = 0,
		SERVER_ACTION_EDIT = 1,
		SERVER_ACTION_REMOVE = 2
	};

	wxString displayName(const SServerListEntry& entry)
	{
		return ServersPanel::displayName(entry);
	}

	wxString lastBackupText(const SServerListEntry& entry)
	{
		if (entry.last_backup <= 0)
			return _("Never");
		return wxDateTime(static_cast<time_t>(entry.last_backup)).Format(wxT("%Y-%m-%d %H:%M"));
	}

	wxStaticText* boldText(wxWindow* parent, const wxString& text)
	{
		wxStaticText* ret = new wxStaticText(parent, wxID_ANY, text);
		wxFont font = ret->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);
		ret->SetFont(font);
		return ret;
	}
}

ClientSettingsDialog::ClientSettingsDialog(wxWindow* parent, const SServerList& server_list, int capa)
	: wxDialog(parent, wxID_ANY, _("Settings"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	entries(server_list.entries), pending(server_list.pending), primary(server_list.primary), modified(false), capa(capa)
{
	wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);

	//The computer name is the same for all servers
	wxBoxSizer* name_row = new wxBoxSizer(wxHORIZONTAL);
	name_row->Add(new wxStaticText(this, wxID_ANY, _("Computer name:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	computername_orig = Settings::currentComputerName();
	m_computername = new wxTextCtrl(this, wxID_ANY, computername_orig, wxDefaultPosition, wxDLG_UNIT(this, wxSize(120, -1)));
	m_computername->SetValidator(getPathValidator());
	name_row->Add(m_computername, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	top->Add(name_row, 0, wxALL, 5);

	wxBoxSizer* servers_header = new wxBoxSizer(wxHORIZONTAL);
	servers_header->Add(boldText(this, _("Servers")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	servers_header->AddStretchSpacer();
	m_trust = new wxButton(this, wxID_ANY, _("Trust new server..."));
	servers_header->Add(m_trust, 0, wxALL, 5);
	wxButton* add = new wxButton(this, wxID_ANY, _("+ Add server"));
	servers_header->Add(add, 0, wxALL, 5);
	top->Add(servers_header, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

	m_servers = new wxPanel(this, wxID_ANY);
	top->Add(m_servers, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

	//The other configuration windows (opened in this process, so without another elevation)
	wxBoxSizer* bottom = new wxBoxSizer(wxHORIZONTAL);
	if (!MyTimer::hasCapability(DONT_ALLOW_CONFIG_PATHS, capa))
	{
		wxButton* btn = new wxButton(this, wxID_ANY, _("Backup paths..."));
		btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Settings::openPaths(this); });
		bottom->Add(btn, 0, wxALL, 5);
	}
	if (!MyTimer::hasCapability(DONT_SHOW_LOGS, capa))
	{
		wxButton* btn = new wxButton(this, wxID_ANY, _("Logs..."));
		btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Settings::openLogs(this); });
		bottom->Add(btn, 0, wxALL, 5);
	}
#ifdef _WIN32
	if (!MyTimer::hasCapability(DONT_ALLOW_COMPONENT_CONFIG, capa))
	{
		wxButton* btn = new wxButton(this, wxID_ANY, _("Components..."));
		btn->SetToolTip(_("Configure components to backup"));
		btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Settings::openComponents(this); });
		bottom->Add(btn, 0, wxALL, 5);
	}
	if (!MyTimer::hasCapability(DONT_ALLOW_COMPONENT_RESTORE, capa)
		&& !MyTimer::hasCapability(STATUS_NO_COMPONENTS, capa))
	{
		wxButton* btn = new wxButton(this, wxID_ANY, _("Restore components..."));
		btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Settings::openRestoreComponents(this); });
		bottom->Add(btn, 0, wxALL, 5);
	}
#endif
	bottom->AddStretchSpacer();
	wxButton* ok = new wxButton(this, wxID_OK, _("Ok"));
	wxButton* cancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
	bottom->Add(ok, 0, wxALL, 5);
	bottom->Add(cancel, 0, wxALL, 5);
	top->Add(bottom, 0, wxEXPAND | wxALL, 5);

	SetSizer(top);

	add->Bind(wxEVT_BUTTON, &ClientSettingsDialog::OnAdd, this);
	m_trust->Bind(wxEVT_BUTTON, &ClientSettingsDialog::OnTrust, this);
	m_servers->Bind(wxEVT_BUTTON, &ClientSettingsDialog::OnServerButton, this);
	ok->Bind(wxEVT_BUTTON, &ClientSettingsDialog::OnOk, this);
	cancel->Bind(wxEVT_BUTTON, &ClientSettingsDialog::OnCancel, this);

	fillServers();
	Centre();
}

void ClientSettingsDialog::fillServers()
{
	m_servers->DestroyChildren();

	wxFlexGridSizer* grid = new wxFlexGridSizer(7, wxDLG_UNIT(this, wxSize(6, 2)));
	grid->Add(boldText(m_servers, _("Name")));
	grid->Add(boldText(m_servers, _("Identity")));
	grid->Add(boldText(m_servers, _("Local")));
	grid->Add(boldText(m_servers, _("Internet")));
	grid->Add(boldText(m_servers, _("Status")));
	grid->Add(boldText(m_servers, _("Last backup")));
	grid->Add(new wxStaticText(m_servers, wxID_ANY, wxEmptyString));

	for (size_t i = 0; i < entries.size(); ++i)
	{
		const SServerListEntry& e = entries[i];
		wxString name = displayName(e);
		if (!primary.empty() && e.ident == primary && entries.size() > 1)
			name += wxT(" ") + _("(primary)");
		grid->Add(new wxStaticText(m_servers, wxID_ANY, name), 0, wxALIGN_CENTER_VERTICAL);
		wxStaticText* fp = new wxStaticText(m_servers, wxID_ANY, fingerprintText(e));
		if (!e.fingerprint.empty())
			fp->SetToolTip(wxString::FromUTF8(e.fingerprint.c_str()));
		grid->Add(fp, 0, wxALIGN_CENTER_VERTICAL);
		grid->Add(new wxStaticText(m_servers, wxID_ANY, e.local ? wxString::FromUTF8("\xE2\x9C\x93") : wxString()), 0, wxALIGN_CENTER);
		grid->Add(new wxStaticText(m_servers, wxID_ANY, e.internet ? wxString::FromUTF8("\xE2\x9C\x93") : wxString()), 0, wxALIGN_CENTER);
		grid->Add(new wxStaticText(m_servers, wxID_ANY, statusText(e)), 0, wxALIGN_CENTER_VERTICAL);
		grid->Add(new wxStaticText(m_servers, wxID_ANY, e.ident.empty() ? wxString(wxT("-")) : lastBackupText(e)), 0, wxALIGN_CENTER_VERTICAL);

		wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
		int base = ID_SERVER_BUTTON + 3 * static_cast<int>(i);
		wxButton* settings = new wxButton(m_servers, base + SERVER_ACTION_SETTINGS, _("Backup settings..."));
		//Backup settings exist once the server has connected (and is saved in the list)
		settings->Enable(!e.ident.empty());
		buttons->Add(settings, 0, wxRIGHT, 3);
		buttons->Add(new wxButton(m_servers, base + SERVER_ACTION_EDIT, _("Edit...")), 0, wxRIGHT, 3);
		buttons->Add(new wxButton(m_servers, base + SERVER_ACTION_REMOVE, _("Remove")), 0);
		grid->Add(buttons, 0, wxALIGN_CENTER_VERTICAL);
	}

	if (entries.empty())
	{
		grid->Add(new wxStaticText(m_servers, wxID_ANY, _("No servers yet. Add one, or wait for a server in the local network to find this computer.")));
	}

	m_servers->SetSizer(grid, true);

	m_trust->Show(!pending.empty());

	m_servers->Layout();
	GetSizer()->Layout();
	GetSizer()->Fit(this);
}

void ClientSettingsDialog::OnServerButton(wxCommandEvent& event)
{
	int id = event.GetId() - ID_SERVER_BUTTON;
	if (id < 0)
	{
		event.Skip();
		return;
	}
	size_t idx = static_cast<size_t>(id / 3);
	int action = id % 3;
	if (idx >= entries.size())
	{
		event.Skip();
		return;
	}

	if (action == SERVER_ACTION_SETTINGS)
	{
		Settings* s = new Settings(this, entries[idx].ident, true, true);
		s->ShowModal();
		s->Destroy();
	}
	else if (action == SERVER_ACTION_EDIT)
	{
		ServerEditDialog dlg(this, entries[idx]);
		if (dlg.ShowModal() == wxID_OK)
		{
			entries[idx] = dlg.getEntry();
			modified = true;
			fillServers();
		}
	}
	else if (action == SERVER_ACTION_REMOVE)
	{
		wxString msg = wxString::Format(_("Remove server \"%s\"? The client will no longer trust it, "
			"so it cannot back up this computer anymore until it is added again."), displayName(entries[idx]));
		if (wxMessageBox(msg, _("Remove server"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
			return;
		entries.erase(entries.begin() + idx);
		modified = true;
		//The buttons are destroyed by fillServers, which is called from their event handler
		CallAfter(&ClientSettingsDialog::fillServers);
	}
}

void ClientSettingsDialog::OnAdd(wxCommandEvent& event)
{
	SServerListEntry entry;
	entry.id = -1;
	entry.local = false;
	entry.internet = true;
	ServerEditDialog dlg(this, entry);
	if (dlg.ShowModal() == wxID_OK)
	{
		entries.push_back(dlg.getEntry());
		modified = true;
		fillServers();
	}
}

void ClientSettingsDialog::OnTrust(wxCommandEvent& event)
{
	if (pending.empty())
		return;

	wxArrayString choices;
	for (size_t i = 0; i < pending.size(); ++i)
	{
		choices.Add(wxString::FromUTF8(pending[i].c_str()));
	}

	int idx = wxGetSingleChoiceIndex(_("These servers tried to back up this computer, but are not trusted. "
		"Only trust a server you know."), _("Trust new server"), choices, this);
	if (idx < 0)
		return;

	if (!Connector::addNewServer(pending[idx]))
	{
		wxMessageBox(_("Trusting the server failed."), _("Trust new server"), wxOK | wxICON_ERROR, this);
		return;
	}

	SServerList server_list = Connector::getServerList();
	if (server_list.supported)
	{
		//Keep local edits, add the newly trusted server
		for (size_t i = 0; i < server_list.entries.size(); ++i)
		{
			bool found = false;
			for (size_t j = 0; j < entries.size(); ++j)
			{
				if (entries[j].id == server_list.entries[i].id)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				entries.push_back(server_list.entries[i]);
			}
		}
		pending = server_list.pending;
	}
	CallAfter(&ClientSettingsDialog::fillServers);
}

void ClientSettingsDialog::OnOk(wxCommandEvent& event)
{
	if (!m_computername->GetValue().empty()
		&& m_computername->GetValue() != computername_orig)
	{
		//settings.cfg (the primary server's file) has the computer name
		if (!Connector::updateSettings("computername=" + std::string(m_computername->GetValue().ToUTF8()) + "\n", 5000, primary))
		{
			wxMessageBox(_("Saving the computer name failed."), wxT("UrBackup"), wxOK | wxICON_ERROR, this);
			return;
		}
	}

	if (modified
		&& !Connector::setServerList(entries))
	{
		wxMessageBox(_("Saving the server list failed."), wxT("UrBackup"), wxOK | wxICON_ERROR, this);
		return;
	}

	EndModal(wxID_OK);
}

void ClientSettingsDialog::OnCancel(wxCommandEvent& event)
{
	if (modified
		&& wxMessageBox(_("Discard the changes to the server list?"), wxT("UrBackup"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
	{
		return;
	}
	EndModal(wxID_CANCEL);
}
