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

#pragma once

#include <wx/wx.h>
#include <wx/listctrl.h>
#include "Connector.h"

//Edits one server of the server list. "Local (LAN)" and "Internet" gate what the
//client does with the server; the internet fields are only enabled with "Internet"
class ServerEditDialog : public wxDialog
{
public:
	ServerEditDialog(wxWindow* parent, const SServerListEntry& entry);

	SServerListEntry getEntry() const { return entry; }

private:
	void OnInternetCheck(wxCommandEvent& event);
	void OnOk(wxCommandEvent& event);
	void updateEnabled();

	SServerListEntry entry;

	wxTextCtrl* m_name;
	wxCheckBox* m_local;
	wxCheckBox* m_internet;
	wxTextCtrl* m_url;
	wxTextCtrl* m_proxy;
	wxTextCtrl* m_authkey;
	wxCheckBox* m_compress;
	wxCheckBox* m_encrypt;
};

//"Servers" page of the settings dialog
class ServersPanel : public wxPanel
{
public:
	ServersPanel(wxWindow* parent, const SServerList& server_list);

	bool isModified() const { return modified; }
	const std::vector<SServerListEntry>& getEntries() const { return entries; }

	static wxString displayName(const SServerListEntry& entry);
	//"urbackup://host:port" from the server/port fields (and back)
	static wxString toUrl(const SServerListEntry& entry);
	static void fromUrl(wxString url, SServerListEntry& entry);

private:
	void fillList();
	long selectedItem();
	void OnAdd(wxCommandEvent& event);
	void OnEdit(wxCommandEvent& event);
	void OnRemove(wxCommandEvent& event);
	void OnTrust(wxCommandEvent& event);
	void OnSelect(wxListEvent& event);
	void OnActivate(wxListEvent& event);
	void updateButtons();

	std::vector<SServerListEntry> entries;
	std::vector<std::string> pending;
	bool modified;

	wxListCtrl* m_list;
	wxButton* m_add;
	wxButton* m_edit;
	wxButton* m_remove;
	wxButton* m_trust;
};
