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
