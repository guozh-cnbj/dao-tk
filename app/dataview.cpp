/*******************************************************************************************************
*  Copyright 2017 Alliance for Sustainable Energy, LLC
*
*  NOTICE: This software was developed at least in part by Alliance for Sustainable Energy, LLC
*  (�Alliance�) under Contract No. DE-AC36-08GO28308 with the U.S. Department of Energy and the U.S.
*  The Government retains for itself and others acting on its behalf a nonexclusive, paid-up,
*  irrevocable worldwide license in the software to reproduce, prepare derivative works, distribute
*  copies to the public, perform publicly and display publicly, and to permit others to do so.
*
*  Redistribution and use in source and binary forms, with or without modification, are permitted
*  provided that the following conditions are met:
*
*  1. Redistributions of source code must retain the above copyright notice, the above government
*  rights notice, this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright notice, the above government
*  rights notice, this list of conditions and the following disclaimer in the documentation and/or
*  other materials provided with the distribution.
*
*  3. The entire corresponding source code of any redistribution, with or without modification, by a
*  research entity, including but not limited to any contracting manager/operator of a United States
*  National Laboratory, any institution of higher learning, and any non-profit organization, must be
*  made publicly available under this license for as long as the redistribution is made available by
*  the research entity.
*
*  4. Redistribution of this software, without modification, must refer to the software by the same
*  designation. Redistribution of a modified version of this software (i) may not refer to the modified
*  version by the same designation, or by any confusingly similar designation, and (ii) must refer to
*  the underlying software originally provided by Alliance as �System Advisor Model� or �SAM�. Except
*  to comply with the foregoing, the terms �System Advisor Model�, �SAM�, or any confusingly similar
*  designation may not be used to refer to any modified version of this software or any modified
*  version of the underlying software originally provided by Alliance without the prior written consent
*  of Alliance.
*
*  5. The name of the copyright holder, contributors, the United States Government, the United States
*  Department of Energy, or any of their employees may not be used to endorse or promote products
*  derived from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
*  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
*  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER,
*  CONTRIBUTORS, UNITED STATES GOVERNMENT OR UNITED STATES DEPARTMENT OF ENERGY, NOR ANY OF THEIR
*  EMPLOYEES, BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
*  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
*  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>

#include <wx/wx.h>
#include <wx/mdi.h>
#include <wx/config.h>
#include <wx/busyinfo.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/scrolwin.h>
#include <wx/clipbrd.h>
#include <wx/busyinfo.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/statline.h>
#include <wx/tglbtn.h>
#include <wx/splitter.h>
#include <wx/grid.h>

#include <wex/plot/plplotctrl.h>
#include <wex/plot/pllineplot.h>
#include <wex/plot/plbarplot.h>
#include <wex/plot/plhistplot.h>
#include <wex/dview/dvplotctrl.h>
#include <wex/dview/dvtimeseriesdataset.h>
#include <wex/extgrid.h>
#include <wex/numeric.h>

#include "dataview.h"
#include "treeselect.h"
#include "project.h"

#ifdef __WXOSX__
#define FONTSIZE 13
#else
#define FONTSIZE 9
#endif

class DataView::Table : public wxGridTableBase
{
public:

	Table()
	{
		m_vt_ref = 0;
		m_attr = new wxGridCellAttr;
		m_attr->SetBackgroundColour( wxColour( 240,240,240 ) );
		m_attr->SetTextColour( "navy" );
		m_attr->SetFont( wxFont(FONTSIZE, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL) );
	}

	virtual ~Table()
	{		
		m_attr->DecRef();
		m_vt_ref = 0;
	}

	

	
    virtual wxGridCellAttr *GetAttr(int , int col,
                                    wxGridCellAttr::wxAttrKind  )
	{
		if (col >= 0 && col < (int)m_items.Count())
		{
			if (!m_vt_ref) return NULL;
			lk::vardata_t *v = m_vt_ref->at( (const char*)m_items[col].c_str() );
			if (!v) return NULL;

			if (v->type() != lk::vardata_t::VECTOR) return NULL;

			m_attr->IncRef();
			return m_attr;
		}
		else
			return NULL;
	}

	void Detach()
	{
		m_vt_ref = 0;
	}
	
	virtual int GetNumberRows()
	{
		int max0 = 0;
		for (int i=0;i<(int)m_items.Count();i++)
		{
			if (!m_vt_ref) continue;
			lk::vardata_t *v = m_vt_ref->at( (const char*)m_items[i].c_str() );
			if (!v) continue;

			int len = 1;
			if (v->type() == lk::vardata_t::VECTOR) 
            {
				if( v->vec()->empty() )
					len = 0;
				else
                {
					if(v->vec()->front().type() == lk::vardata_t::VECTOR)
						len = v->vec()->front().vec()->size();
					else
						len = v->vec()->size();
                }
            }

			if (len > max0) max0 = len;
		}
		return max0;
	}

	virtual int GetNumberCols()
	{
		return m_items.Count();
	}

	virtual bool IsEmptyCell(int row, int col)
	{
		if ( col < 0 || col >= (int)m_items.Count() || row < 0 ) return true;
		
		if (!m_vt_ref) return true;
		lk::vardata_t *v = m_vt_ref->at( (const char*)m_items[col].c_str() );
		if (!v) return true;

		if ( v->type() == lk::vardata_t::STRING && row >= 1 ) return true;

		if (v->type() == lk::vardata_t::VECTOR) 
			if( row >= (int)v->vec()->size())
				return true;

		return false;
	}

	virtual wxString GetValue( int row, int col )
	{
		if (m_vt_ref && col >= 0 && col < (int)m_items.Count())
		{
			lk::vardata_t *v = m_vt_ref->at( (const char*)m_items[col].c_str() );
			if (!v) return "<lookup error>";

			if (v->type() == lk::vardata_t::STRING && row == 0) return wxString(v->as_string().c_str());
			
			else if (v->type() == lk::vardata_t::NUMBER && row == 0) return wxString::Format("%lf", v->as_number() );
			
			else if (v->type() == lk::vardata_t::VECTOR && row < (int)v->vec()->size())
			{
				if ( v->vec()->empty() )
					return "";

				if (
						v->type() == lk::vardata_t::VECTOR 
						&& v->vec()->front().type() != lk::vardata_t::VECTOR 
						&& row < (int)v->vec()->size() 
					)
					return wxString::Format("%lf", v->vec()->at(row).as_number() );
				
				else if ( v->vec()->front().type() == lk::vardata_t::VECTOR) 
				{
					wxString ret;
					for (int j=0;j<(int)v->vec()->front().vec()->size();j++)
					{
						ret += wxString::Format("%*lf", 13, v->vec()->at(row).vec()->at(j).as_number() );
					}

					return ret;
				}
			}
            else if (v->type() == lk::vardata_t::HASH)
            {
                variable* vv = static_cast<variable*>(v);
                if (vv->hash_vector.item_count() == 0 || vv->hash_vector.iteration_count() == 0)
                    return "";
                
                wxString ret;

                for (int j = 0; j < (int)vv->hash_vector.item_count(); j++)
                    ret += wxString::Format("%10s", vv->hash_vector.at_index(j).first.c_str());
                ret += "\n";

                for (int j = 0; j < (int)vv->hash_vector.item_count(); j++)
                {
                    for (int i = 0; i < (int)vv->hash_vector.iteration_count(); i++)
                    {
                        ret += wxString::Format("%10.7f", vv->hash_vector.at_index(j).second.at(i));
                    }
                    ret += "\n";
                }
                /*FILE *f = fopen("C:/Users/mwagner/Documents/NREL/projects/dao-tk/deploy/x64/errlog.txt", "a");
                fprintf(f, "%s", ret.ToStdString().c_str());
                fclose(f);*/
                return ret;
            }

		}
		
		return wxEmptyString;
	}

	virtual wxString GetColLabelValue(int col)
	{
		if (col >= 0 && col < (int)m_items.Count())
		{

			if (!m_vt_ref) return m_items[col];
			else return m_items[col];
		}
		else
			return "<unknown>";
	}
	
	void SetData( const wxArrayString &items, lk::varhash_t *vt, bool )
	{
		m_items = items;
		m_vt_ref = vt;
	}

	virtual void SetValue(int,int,const wxString &)
	{
		/* nothing to do */
	}

private:
    wxGridCellAttr *m_attr;
	lk::varhash_t *m_vt_ref;
	wxArrayString m_items;
};

class TreeItemData : public wxTreeItemData
{
public:
	TreeItemData() : wxTreeItemData() 
	{
		varname = "";
	};
	TreeItemData( const char * _varname ) : wxTreeItemData() 
	{
		varname = _varname;
	};

	std::string varname;
};

enum { ID_COPY_CLIPBOARD = 2315,
	   ID_DATA_SEARCH,
	   ID_LIST,
	   ID_SHOW_STATS,
	   ID_SELECT_ALL,
	   ID_UNSELECT_ALL,
	   ID_EXPAND_ALL,
	   ID_COLLAPSE_ALL,
	   ID_DVIEW,
	   ID_POPUP_STATS,
	   ID_POPUP_PLOT_BAR,
	   ID_POPUP_PLOT_LINE,
	   ID_GRID };

BEGIN_EVENT_TABLE( DataView, wxPanel )
	EVT_TEXT( ID_DATA_SEARCH, DataView::OnTextSearch )
	EVT_BUTTON( ID_COPY_CLIPBOARD, DataView::OnCommand )
	EVT_BUTTON( ID_UNSELECT_ALL, DataView::OnCommand )
	EVT_BUTTON( ID_SELECT_ALL, DataView::OnCommand )
	EVT_BUTTON( ID_UNSELECT_ALL, DataView::OnCommand )
	EVT_BUTTON( ID_EXPAND_ALL, DataView::OnCommand )
	EVT_BUTTON( ID_COLLAPSE_ALL, DataView::OnCommand )
	EVT_BUTTON( ID_SHOW_STATS, DataView::OnCommand )
	EVT_BUTTON( ID_DVIEW, DataView::OnCommand )
	EVT_TREE_STATE_IMAGE_CLICK( ID_LIST, DataView::OnVarListCheck )
	EVT_LISTBOX_DCLICK( ID_LIST, DataView::OnVarListDClick )
	EVT_GRID_CMD_LABEL_RIGHT_CLICK( ID_GRID, DataView::OnGridLabelRightClick )
	EVT_GRID_CMD_LABEL_LEFT_DCLICK( ID_GRID, DataView::OnGridLabelDoubleClick )
	
	EVT_MENU( ID_POPUP_STATS, DataView::OnPopup )
	EVT_MENU( ID_POPUP_PLOT_BAR, DataView::OnPopup )
	EVT_MENU( ID_POPUP_PLOT_LINE, DataView::OnPopup )

END_EVENT_TABLE()


DataView::DataView( wxWindow *parent, const char *imagedir ) 
	: wxPanel( parent ),
  	m_frozen(false),
	m_grid_table(0),
	// m_root_item(0),
	m_vt(0)
{
	wxBoxSizer *tb_sizer = new wxBoxSizer(wxHORIZONTAL);
	tb_sizer->Add( new wxButton(this, ID_EXPAND_ALL, "Expand all", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT), 0, wxALL|wxEXPAND, 2);
	tb_sizer->Add( new wxButton(this, ID_COLLAPSE_ALL, "Collapse all", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT), 0, wxALL|wxEXPAND, 2);
	tb_sizer->Add( new wxButton(this, ID_SELECT_ALL, "Select all", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT), 0, wxALL|wxEXPAND, 2);
	tb_sizer->Add( new wxButton(this, ID_UNSELECT_ALL, "Unselect all", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT), 0, wxALL|wxEXPAND, 2);
	tb_sizer->Add( new wxButton( this, ID_COPY_CLIPBOARD, "Copy to clipboard", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT), 0, wxEXPAND|wxALL, 2);
	tb_sizer->Add( new wxButton( this, ID_SHOW_STATS, "Show stats...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT), 0, wxEXPAND|wxALL, 2);
	tb_sizer->Add( new wxButton( this, ID_DVIEW, "Timeseries graph...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT), 0, wxEXPAND|wxALL, 2);
	tb_sizer->AddStretchSpacer(1);

	wxSplitterWindow *splitwin = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE ); 
	splitwin->SetMinimumPaneSize(210);

	wxFont datafont = wxFont(FONTSIZE, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

	wxPanel *vtpanel = new wxPanel(splitwin);
	VarTreeTextCtrl *vtsearch = new VarTreeTextCtrl(vtpanel, ID_DATA_SEARCH);
	
	m_varlist = new VarTreeView( vtpanel, ID_LIST,  imagedir);
	m_varlist->SetFont( datafont );
	m_varlist->SetIndent(8);
	
	wxBoxSizer *vtsizer = new wxBoxSizer(wxVERTICAL);
	vtsizer->Add( vtsearch, 0, wxALL|wxEXPAND, 2);
	vtsizer->Add(m_varlist, 1, wxALL|wxEXPAND, 2);
	vtpanel->SetSizer(vtsizer);
	
	m_grid = new wxExtGridCtrl(splitwin, ID_GRID);
	m_grid->SetFont( datafont );
	m_grid->SetDefaultCellFont( datafont );
	m_grid->SetLabelFont( datafont );
	
	m_grid->EnableEditing(false);
	m_grid->EnableCopyPaste(false);
	m_grid->DisableDragCell();
	m_grid->DisableDragRowSize();
	m_grid->DisableDragColMove();
	m_grid->DisableDragGridSize();
	m_grid->SetDefaultCellAlignment( wxALIGN_RIGHT, wxALIGN_CENTER );
	m_grid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	splitwin->SplitVertically(vtpanel, m_grid, 390);


	wxBoxSizer *szv_main = new wxBoxSizer(wxVERTICAL);
	szv_main->Add( tb_sizer, 0, wxALL|wxEXPAND, 2 );
	//szv_main->Add( new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL ), 0, wxALL|wxEXPAND, 1);
	szv_main->Add( splitwin, 1, wxALL|wxEXPAND, 0 );

	SetSizer( szv_main );

}	

std::vector<int> DataView::GetColumnWidths()
{
	std::vector<int> list;
	for (int i=0;i<m_grid->GetNumberCols();i++)
		list.push_back( m_grid->GetColSize( i ) );
	return list;
}

void DataView::SetColumnWidths( const std::vector<int> &cwl )
{
	for (int i=0;i<(int)cwl.size() && i<m_grid->GetNumberCols();i++)
		m_grid->SetColSize( i, cwl[i] );
}

wxArrayString DataView::GetSelections()
{
	return m_selections;
}

void DataView::SetSelections(const wxArrayString &sel, const wxArrayString &labels)
{
    m_varlist->DeleteAllItems();

    m_root = m_varlist->AddRoot("Data", VarTreeView::ICON_REMOVE,VarTreeView::ICON_REMOVE);
    m_varlist->SetItemBold(m_root);
    wxTreeItemId cur_parent = m_root;
    wxString cur_context = wxEmptyString;
	wxArrayString cur_context_list;
	

    for (int i=0;i < (int)m_names.Count();i++)
    {
		if(  m_names[i].IsEmpty() )
			continue;
		
		// lk::vardata_t *v = m_vt->at(m_names[i]);
		
		wxArrayString parsename = wxSplit(labels[i], wxChar('@'));

		wxString cxt = parsename[0];
        wxString lbl = parsename[1];


        if (cur_context != cxt) 	//something in the context has changed
        {
			wxArrayString cxt_list = wxSplit(parsename[0], wxChar('|'));

			//need to figure out at which level the first difference appears
			int j_level = 0;
			
			int ncxt_list = cxt_list.Count();
			int ncur_context_list = cur_context_list.Count();

			for(int j=0; j<ncxt_list; j++)
			{
				if( j > ncur_context_list-1 )
				{
					j_level = j;
					break;
				}
				if( cxt_list[j] != cur_context_list[j] )
				{
					j_level = j;
					break;
				}
			}

			int nlevels_up = cur_context_list.Count() - j_level;
			
			for( int ind=0; ind<nlevels_up; ind++)
				cur_parent = m_varlist->GetItemParent(cur_parent);

			if( cur_parent == 0 )
				cur_parent = m_root;	//needed on first pass through

			//reconstruct nodes back down
			for( int ind=j_level; ind<ncxt_list; ind++)
			{
				cur_parent = m_varlist->AppendItem(cur_parent, cxt_list[ind], -1, -1, new TreeItemData(cxt.c_str()) );
            	m_varlist->SetItemBold(cur_parent);
			}
		
			cur_context = cxt;
			cur_context_list = cxt_list;
        }
        
		wxTreeItemId item_id;
        if (cur_parent.IsOk())
            item_id = m_varlist->AppendItem( cur_parent, lbl, VarTreeView::ICON_CHECK_FALSE,-1, new TreeItemData(m_names[i].c_str()) );
        else
            item_id = m_varlist->AppendItem( m_root, lbl, VarTreeView::ICON_CHECK_FALSE, -1, new TreeItemData(m_names[i].c_str()) );

        if ( sel.Index( m_names[i] ) >= 0 )
            m_varlist->Check( item_id, true );
    }

    m_varlist->Expand(m_root);
    
    wxTreeItemIdValue cookie; //unused
    wxTreeItemId node = m_varlist->GetFirstChild(m_varlist->GetRootItem(), cookie);
    while (node.IsOk())
    {
        m_varlist->CollapseAllChildren(node);
        node = m_varlist->GetNextSibling(node);
    }
    m_varlist->EnsureVisible(m_varlist->GetRootItem());

    m_varlist->UnselectAll();

}

static void SortByLabels(wxArrayString &names, wxArrayString &labels)
{
	// sort the selections by labels
	wxString buf;
	int count = (int)labels.Count();
	for (int i=0;i<count-1;i++)
	{
		int smallest = i;

		for (int j=i+1;j<count;j++)
			if ( labels[j] < labels[smallest] )
				smallest = j;

		// swap
		buf = labels[i];
		labels[i] = labels[smallest];
		labels[smallest] = buf;

		buf = names[i];
		names[i] = names[smallest];
		names[smallest] = buf;

	}
}

void DataView::Freeze()
{
	m_frozen = true;	
}

void DataView::Thaw()
{
	m_frozen = false;
	UpdateView();
}

void DataView::UpdateView()
{
	if ( m_frozen ) return;

	wxArrayString sel_list = m_selections;

	m_names.Clear();

	// m_varlist->Clear();
	wxArrayString labels;

	if (m_vt != NULL)
	{
		int padto = 0;
		
		std::vector< lk_string > varnames;
		for(lk::varhash_t::iterator it = m_vt->begin(); it != m_vt->end(); it++ )
		{
			varnames.push_back( it->first );

			data_base* vit = static_cast<data_base*>(it->second);
			if( vit->is_shown_in_list )
			{
				int len = vit->GetDisplayName().length();
				if (len > padto) padto = len;
			}
		}
		padto += 2;


		for( int ni=0; ni<(int)varnames.size(); ni++)
		{
			lk_string name = varnames.at(ni);


			if (lk::vardata_t *v = m_vt->at(name))
			{
				data_base *dv = static_cast<data_base*>(v);
				
				if( dv->nice_name.empty() || !dv->is_shown_in_list ) 
					continue;

				wxString label = wxString::Format( "%s@%" + wxString::Format("-%ds",padto) , dv->group, dv->nice_name );

				// if( label.Find("Par") != wxNOT_FOUND)
				// 	double xadsf = 0.;

				m_names.Add( name );
				// for (int j=0;j< padto-(int)label.length();j++)
				// 	label += " ";
				
				if (dv->type() == lk::vardata_t::NUMBER)
					label += "number " + wxString::Format("%lg", v->as_number() );				
				else if (dv->type() == lk::vardata_t::STRING)
					label += "string " + wxString(v->as_string().c_str());
				else if (dv->type() == lk::vardata_t::VECTOR)
                {
					if( v->vec()->empty() )
						label += wxString::Format( "array [%d]", (int)v->vec()->size() );
					else
                    {
						if( v->vec()->front().type() == lk::vardata_t::VECTOR)
							label += wxString::Format("matrix [%d,%d]", (int)v->vec()->front().vec()->size(), (int)v->vec()->size());
						else
							label += wxString::Format( "array [%d]", (int)v->vec()->size() );
                    }
                }
                else if (dv->type() == lk::vardata_t::HASH)
                {
                    ordered_hash_vector* ov = &(static_cast<variable*>(dv))->hash_vector;
                    label += "table " + wxString::Format("[%d x %d]", (int)ov->item_count(), (int)ov->iteration_count());
                }

   				labels.Add( label );
			}

		}

		SortByLabels(m_names, labels );
	}
	
	SetSelections( sel_list, labels );
	UpdateGrid();
}
	
void DataView::UpdateGrid()
{
	std::vector<int> cwl = GetColumnWidths();
	m_grid->Freeze();
	
	if (m_grid_table) m_grid_table->Detach();

	m_grid_table = new DataView::Table;
	m_grid_table->SetData( m_selections, m_vt, true );
	m_grid->SetTable( m_grid_table, true );
	m_grid->SetRowLabelSize(60);
	//m_grid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_grid->Thaw();
	
	m_grid->Layout();
	m_grid->GetParent()->Layout();
	SetColumnWidths(cwl);
	m_grid->ForceRefresh();
//	m_grid->AutoSizeColumns();

}

void DataView::OnCommand(wxCommandEvent &evt)
{
	switch(evt.GetId())
	{
	case ID_SHOW_STATS:
		ShowStats();
		break;
	case ID_DVIEW:
		{
			if( m_selections.size() == 0)
			{
				wxMessageBox("Please select one or more variables.");
				return;
			}

			wxDialog dlg(this, -1, "Timeseries Viewer", wxDefaultPosition, wxSize(900,600), wxRESIZE_BORDER|wxDEFAULT_DIALOG_STYLE);
			wxDVPlotCtrl *dv = new wxDVPlotCtrl( &dlg );
			wxBoxSizer *sz = new wxBoxSizer(wxVERTICAL);
			sz->Add( dv, 1, wxALL|wxEXPAND, 0 );
			sz->Add( dlg.CreateButtonSizer(wxOK), 0, wxALL|wxEXPAND, 0 );
			dlg.SetSizer(sz);
			
			std::vector<double> da(8760);
			int iadded = 0;
			for (size_t i=0;i<m_selections.Count();i++)
			{
				lk::vardata_t *v = m_vt->at( (const char*) m_selections[i].c_str() );
				if ( v != 0 
					&& v->type() == lk::vardata_t::VECTOR
					&& v->vec()->size() % 8760 == 0)
				{
					int nyear = v->vec()->size() / 8760;

					for (int y=0;y<nyear;y++)
					{
						for (int k=0;k<8760;k++)
							da[k] = v->vec()->at(k+y*8760).as_number();

						dv->AddDataSet(  new wxDVArrayDataSet( m_selections[i] + (nyear > 1 ? wxString::Format("_y%d", y+1) : ""), da ) );
						iadded++;
					}

				}
			}
			
			if (iadded == 0)
				wxMessageBox("Please check one or more array variables with a multiple of 8760 values to show in the timeseries viewer.");
			else
			{
				dv->SelectDataOnBlankTabs();
				dlg.ShowModal();
			}

		}
		break;
	case ID_SELECT_ALL:
		{
			m_selections.Clear();
			for(lk::varhash_t::iterator it = m_vt->begin(); it != m_vt->end(); it++ )
				m_selections.Add( it->first );
			UpdateView();
		}
		break;
	case ID_UNSELECT_ALL:
		m_selections.Clear();
		UpdateView();
		break;
	case ID_COPY_CLIPBOARD:
		m_grid->Copy( m_grid->NumCellsSelected() == 1, true);
		break;
	case ID_EXPAND_ALL:
		m_varlist->ExpandAll();
		m_varlist->EnsureVisible( m_varlist->GetRootItem() );
		break;
	case ID_COLLAPSE_ALL:
		{
			wxTreeItemIdValue cookie; //unused
			wxTreeItemId node = m_varlist->GetFirstChild( m_varlist->GetRootItem(), cookie);
			while( node.IsOk() )
			{
				m_varlist->CollapseAllChildren( node );
				node = m_varlist->GetNextSibling(node);
			}
			m_varlist->EnsureVisible( m_varlist->GetRootItem() );
		}
	}
}


void DataView::OnTextSearch( wxCommandEvent & evt)
{
    wxString filter = ( static_cast<VarTreeTextCtrl*>(evt.GetEventObject()) )->GetValue().Lower();

	if (filter.IsEmpty())
    {
        for (lk::varhash_t::iterator it= m_vt->begin(); it != m_vt->end(); it++)
            static_cast<data_base*>( it->second )->is_shown_in_list = true;
    }
    else
    {
        for (lk::varhash_t::iterator it= m_vt->begin(); it != m_vt->end(); it++)
        {
            data_base *vit = static_cast<data_base*>( it->second );
			wxString searchname = wxString(vit->group + vit->name + vit->GetDisplayName());	//search group, variable, and display name

            if (filter.Len() <= 2 && searchname.Left( filter.Len() ).Lower() == filter)
                vit->is_shown_in_list = true;
            else if (searchname.Lower().Find( filter ) >= 0)
                vit->is_shown_in_list = true;
            // else if (searchname.Lower().Find( filter ) == 0)
            //     vit->is_shown_in_list = true;
			else if ( m_selections.Index( vit->name ) != wxNOT_FOUND )
				vit->is_shown_in_list = true;
            else
                vit->is_shown_in_list = false;
        }
    }

    UpdateView();

    if ( !filter.IsEmpty() )
    {
        m_varlist->ExpandAll();
        m_varlist->EnsureVisible( m_root );
    }
    else
    {
        wxTreeItemId current_item; // = m_varlist->GetRootItem();
		wxTreeItemIdValue cookie; 		//unused, but required for the call
        current_item = m_varlist->GetFirstChild(m_varlist->GetRootItem(), cookie);

		while( current_item.IsOk() )
		{
			if( m_varlist->IsChecked(current_item) )
				m_varlist->Expand( m_varlist->GetItemParent( current_item ) );

			current_item = m_varlist->GetNextChild( current_item, cookie );
		}

    }
}


void DataView::OnVarListCheck(wxTreeEvent &evt)
{
	
	wxTreeItemId idx = evt.GetItem();

	// if (idx >= 0  && idx < (int)m_names.Count())
	// {
		wxString var = static_cast<TreeItemData*>( m_varlist->GetItemData(idx) )->varname;
		
		if (m_varlist->IsChecked(idx) && m_selections.Index(var) == wxNOT_FOUND)
			m_selections.Add( var );

		if (!m_varlist->IsChecked(idx) && m_selections.Index(var) != wxNOT_FOUND)
			m_selections.Remove( var );
	// }

	UpdateGrid();
}

void DataView::OnVarListDClick(wxCommandEvent &)
{
	ShowStats( wxEmptyString );
}

wxString DataView::GetSelection()
{
	return ( static_cast<TreeItemData*>( m_varlist->GetItemData( m_varlist->GetSelection() ) ) )->varname;

	// int n = m_varlist->GetSelection();
	// if (n >= 0 && n < (int)m_names.Count())
	// 	return m_names[n];
	// else
	// 	return wxEmptyString;
}

void DataView::ShowStats( wxString name )
{
	wxString usename = name;
	if (name.IsEmpty()) 
	{
		if( m_selections.size() == 0 )
		{
			wxMessageBox("Please select a variable.");
			return;
		}
		else
		{
			usename = m_selections.back();
		}
	}

	if (m_vt)
	{
		lk::vardata_t *v = m_vt->at((const char*) usename.c_str() );
		if (!v)
		{
			wxMessageBox("variable not found.");
			return;
		}
        //if it's a hash vector or table, show the listing in a popup. Otherwise, show stats for an array type.
        /*if (v->type() == lk::vardata_t::VECTOR)
        {
		    StatDialog dlg(this, "Stats for: " + usename);
		    dlg.Compute( *v->vec() );
		    dlg.ShowModal();
        }*/
        else if (v->type() == lk::vardata_t::HASH || v->type() == lk::vardata_t::VECTOR)
        {
            //call the hash popup dialog
            TableViewDialog dlg(this, static_cast<variable*>(v), "Variable data", wxID_ANY, wxDEFAULT_DIALOG_STYLE|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxRESIZE_BORDER);
            dlg.SetSize(wxSize(this->GetSize().GetWidth()*0.75, this->GetSize().GetHeight()*.75));
            dlg.ShowModal();
            dlg.CenterOnParent();
        }
        else
        {
            wxMessageBox("variable not of array or table type.");
        }
	}
}

void DataView::OnGridLabelRightClick(wxGridEvent &evt)
{
	int col = evt.GetCol();
	if (col < 0 || col >= (int)m_selections.Count()) return;
	
	m_popup_var_name = m_selections[col];

	wxMenu popup;
	popup.Append( ID_POPUP_STATS, "Statistics...");
	popup.Append( ID_POPUP_PLOT_BAR, "Bar plot (array only)" );
	popup.Append( ID_POPUP_PLOT_LINE, "Line plot (array only)" );

	m_grid->PopupMenu( &popup, evt.GetPosition() );
}

void DataView::OnGridLabelDoubleClick(wxGridEvent &evt)
{
	int col = evt.GetCol();
	if (col < 0 || col >= (int)m_selections.Count()) return;
	ShowStats( m_selections[col] );
}

void DataView::OnPopup(wxCommandEvent &evt)
{
	switch(evt.GetId())
	{
	case ID_POPUP_STATS:
		ShowStats( m_popup_var_name );
		break;
	case ID_POPUP_PLOT_BAR:
	case ID_POPUP_PLOT_LINE:
		{
			if (!m_vt) return;
			lk::vardata_t *v = m_vt->at( (const char*) m_popup_var_name.c_str() );
			if (!v || v->type() != lk::vardata_t::VECTOR)
			{
				wxMessageBox("variable not found or not of array type.");
				return;
			}

			wxFrame *frm = new wxFrame(this, -1, "plot: " + m_popup_var_name, wxDefaultPosition, wxSize(500,350));

			if ( v->vec()->size() == 8760 )
			{
				wxDVPlotCtrl *dv = new wxDVPlotCtrl( frm );
				std::vector<double> da(8760);
				for (int i=0;i<8760;i++)
					da[i] = v->vec()->at(i).as_number();

				dv->AddDataSet(  new wxDVArrayDataSet( m_popup_var_name, da ) );
			}
			else
			{
				wxPLPlotCtrl *plotsurf = new wxPLPlotCtrl( frm, wxID_ANY );
			
				double minval = 1e99;
				double maxval = 1e-99;
				std::vector<wxRealPoint> pdat;
				for (int i=0;i<(int)v->vec()->size(); i++)
				{
					double vali = v->vec()->at(i).as_number();

					pdat.push_back( wxRealPoint( i+1, vali ) );
					if ( vali < minval ) minval = vali;
					if ( vali > maxval ) maxval = vali;
				}

				
				if( evt.GetId() == ID_POPUP_PLOT_LINE )
				{
					double range = maxval-minval;
					plotsurf->AddPlot( new wxPLLinePlot( pdat, m_popup_var_name ) );
					minval -= (0.05*range);
					maxval += (0.05*range);
					plotsurf->SetYAxis1( new wxPLLinearAxis( minval, maxval ) );
				}
				else
				{
					plotsurf->AddPlot( new wxPLBarPlot( pdat, 0.0, m_popup_var_name ) );
					if ( minval > 0 ) minval = 0;
					if ( maxval < 0 ) maxval = 0;
					plotsurf->SetYAxis1( new wxPLLinearAxis( minval, maxval ) );
				}

				plotsurf->SetTitle("Plot of: '" + m_popup_var_name + "'");
				plotsurf->SetXAxis1( new wxPLLinearAxis( 0, v->vec()->size()+1 ) );

			}

			frm->Show();
		}
		break;
	}
}


#include <wex/numeric.h>
#include <wex/extgrid.h>

StatDialog::StatDialog( wxWindow *parent, const wxString &title )
	 : wxDialog( parent, wxID_ANY, title, 
		wxDefaultPosition, wxSize( 640,480), 
		wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER )
{
	wxBoxSizer *sz_h1 = new wxBoxSizer( wxHORIZONTAL );
	
	sz_h1->Add( new wxStaticText( this, wxID_ANY, "Mean:" ), 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	sz_h1->Add( numMean = new wxTextCtrl(this, wxID_ANY) );
	sz_h1->Add( new wxStaticText( this, wxID_ANY, "Min:" ), 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	sz_h1->Add( numMin = new wxTextCtrl(this, wxID_ANY) );
	sz_h1->Add( new wxStaticText( this, wxID_ANY, "Max:" ), 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	sz_h1->Add( numMax = new wxTextCtrl(this, wxID_ANY) );

	wxBoxSizer *sz_h2 = new wxBoxSizer( wxHORIZONTAL );

	sz_h2->Add( new wxStaticText( this, wxID_ANY, "Sum:" ), 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	sz_h2->Add( numSum = new wxTextCtrl(this, wxID_ANY) );
	sz_h2->Add( new wxStaticText( this, wxID_ANY, "Sum/1000:" ), 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	sz_h2->Add( numSumOver1000 = new wxTextCtrl(this, wxID_ANY) );
	
	grdMonthly = new wxExtGridCtrl(this, wxID_ANY);
	grdMonthly->CreateGrid(12,4);
	grdMonthly->EnableEditing(false);
	grdMonthly->DisableDragCell();
	grdMonthly->DisableDragColSize();
	grdMonthly->DisableDragRowSize();
	grdMonthly->DisableDragColMove();
	grdMonthly->DisableDragGridSize();
	grdMonthly->SetRowLabelSize(23);
	grdMonthly->SetColLabelSize(23);

	wxBoxSizer *sz_main = new wxBoxSizer( wxVERTICAL );
	sz_main->Add( sz_h1 );	
	sz_main->Add( sz_h2 );
	sz_main->Add( grdMonthly, 1, wxALL|wxEXPAND, 5 );
	sz_main->Add( CreateButtonSizer( wxOK ), 0, wxALL|wxEXPAND, 5 );

	SetSizer( sz_main );
}

void StatDialog::Compute( std::vector<lk::vardata_t> &val )
{
static int nday[12] = {31,28,31,30,31,30,31,31,30,31,30,31}; 

	size_t len = val.size();
	// double *pvals = val.data();

	double min, max, mean, sum;
	double mmin[12],mmax[12],mmean[12],msum[12];

	size_t i,j;
	min = (double)1e19;
	max = (double)-1e19;
	mean=sum=0.0;
	for (i=0;i<12;i++)
	{
		mmin[i]=(double)1e19;
		mmax[i]=(double)-1e19;
		mmean[i]=msum[i] = 0;
	}


	for (i=0;i<len;i++)
	{
		double vali = val.at(i).as_number();

		if (vali < min) min = vali;
		if (vali > max) max = vali;
		sum += vali;
	}

	mean = sum/((double)len);
	if( mean != mean )
		mean = (min+max)/2.;

	numMin->SetValue( wxString::Format("%f",min) );
	numMax->SetValue(wxString::Format("%f", max));
	numMean->SetValue(wxString::Format("%f", mean));
	numSum->SetValue(wxString::Format("%f", sum));
	numSumOver1000->SetValue(wxString::Format("%f", sum/1000.0) );

	size_t multiple = len / 8760;
	if ( multiple*8760 == len )
	{
		i=0;
		for (int m=0;m<12;m++)
		{
			for (int d=0;d<nday[m];d++)
			{
				for (int h=0;h<24;h++)
				{
					double aval = 0.0;
					for (j=0;j<multiple;j++)
						aval += val[i*multiple+j].as_number();


					if (aval < mmin[m]) mmin[m] = aval;
					if (aval > mmax[m]) mmax[m] = aval;
					msum[m] += aval;

					i++;
				}
			}

			mmean[m] = msum[m] / ( nday[m]*24 );
		}
	}

	grdMonthly->ResizeGrid(12,5);
	for (i=0;i<12;i++)
	{
		grdMonthly->SetCellValue( i, 0, wxString::Format("%lg", mmin[i]  )  );
		grdMonthly->SetCellValue( i, 1, wxString::Format("%lg", mmax[i]  )  );
		grdMonthly->SetCellValue( i, 2, wxString::Format("%lg", mmean[i] )  );
		grdMonthly->SetCellValue( i, 3, wxString::Format("%lg", msum[i] ) );
		grdMonthly->SetCellValue( i, 4, wxString::Format("%lg", msum[i]/1000.0f ) );
	}
	
	grdMonthly->SetRowLabelValue(0, "Jan");
	grdMonthly->SetRowLabelValue(1, "Feb");
	grdMonthly->SetRowLabelValue(2, "Mar");
	grdMonthly->SetRowLabelValue(3, "Apr");
	grdMonthly->SetRowLabelValue(4, "May");
	grdMonthly->SetRowLabelValue(5, "Jun");
	grdMonthly->SetRowLabelValue(6, "Jul");
	grdMonthly->SetRowLabelValue(7, "Aug");
	grdMonthly->SetRowLabelValue(8, "Sep");
	grdMonthly->SetRowLabelValue(9, "Oct");
	grdMonthly->SetRowLabelValue(10, "Nov");
	grdMonthly->SetRowLabelValue(11, "Dec");

	grdMonthly->SetColLabelValue(0, "Min");
	grdMonthly->SetColLabelValue(1, "Max");
	grdMonthly->SetColLabelValue(2, "Mean");
	grdMonthly->SetColLabelValue(3, "Sum");
	grdMonthly->SetColLabelValue(4, "Sum/1000");

	grdMonthly->SetRowLabelSize( 40 );
	grdMonthly->SetColLabelSize( wxGRID_AUTOSIZE );
}

//--------------------------------------------------------------
enum
{
    ID_TVD_GRID,
    ID_TVD_COPY,
    ID_TVD_EXPORT
};

TableViewDialog::TableViewDialog(wxWindow *parent, variable *vardata, wxString title, int id, long style)
    : wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, style)
{
    //_keyboardstate = new wxKeyboardState();

    _vardata = vardata;

    int nrow=0, ncol=0;

    //find the type
    if (vardata->type() == lk::vardata_t::VECTOR)
    {
        if (vardata->vec()->begin()->type() == lk::vardata_t::VECTOR)
        {
            _type = TableViewDialog::TVD_ARRAY;
            nrow = vardata->vec()->size();
            ncol = vardata->vec()->front().vec()->size();
        }
    }
    else if (vardata->type() == lk::vardata_t::HASH)
    {
        if (vardata->hash_vector.item_count() > 0)
        {
            _type = TableViewDialog::TVD_VECTHASH;
            nrow = vardata->hash_vector.iteration_count();
            ncol = vardata->hash_vector.item_count();
        }
        else
        {
            _type = TableViewDialog::TVD_TABLE;
            nrow = vardata->hash()->size();
            if (nrow > 0)
                ncol = vardata->hash()->begin()->second->vec()->size();
        }
    }
    else
    {
        return;
    }

    _grid = new wxGrid(this, ID_TVD_GRID);
    _grid->CreateGrid(nrow, ncol);
    _grid->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(TableViewDialog::OnCopy), 0, this);
    _grid->SetSelectionMode(wxGrid::wxGridSelectionModes::wxGridSelectRowsOrColumns);

    //set the grid values
    switch (_type)
    {
    case TableViewDialog::TVD_ARRAY:
    {
        for (size_t i = 0; i < nrow; i++)
        {
            std::vector<lk::vardata_t>* vrow = vardata->vec()->at(i).vec();
            for (size_t j = 0; j < vrow->size(); j++)
            {
                _grid->SetCellValue(i, j, wxString::Format("%f", vrow->at(j).as_number()));
            }
        }
        break;
    }
    case TableViewDialog::TVD_TABLE:
    {
        int i = 0;
        for (lk::varhash_t::iterator vit = vardata->hash()->begin(); vit != vardata->hash()->end(); vit++)
        {
            _grid->SetColLabelValue(i, vit->first);
            std::vector<lk::vardata_t>* vec = vit->second->vec();
            for (size_t j = 0; j < vit->second->vec()->size(); j++)
            {
                _grid->SetCellValue(j, i, wxString::Format("%f", vec->at(j).as_number()));
            }
            i++;
        }
        break;
    }
    case TableViewDialog::TVD_VECTHASH:
    {
        for (size_t i = 0; i < ncol; i++)
        {
            _grid->SetColLabelValue(i, vardata->hash_vector.at_index(i).first);
            std::vector<double> *vdat = &vardata->hash_vector.at_index(i).second;
            
            for (size_t j = 0; j < nrow; j++)
                _grid->SetCellValue(j, i, wxString::Format("%f", vdat->at(j)));
        }
    }
    default:
        break;
    }

    wxBoxSizer *bsizer = new wxBoxSizer(wxHORIZONTAL);
    bsizer->Add(new wxButton(this, ID_TVD_COPY, "Copy"), 0, wxALL, 5);
    bsizer->Add(new wxButton(this, ID_TVD_EXPORT, "Export..."), 0, wxALL, 5);

    wxBoxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);
    mainsizer->Add(bsizer);

    mainsizer->Add(new wxStaticText(this, wxID_ANY, vardata->nice_name + " [" + vardata->units + "]"), 0, wxALL, 5);
    mainsizer->Add(_grid, 1, wxEXPAND | wxALL, 5);
    mainsizer->SetSizeHints(_grid);
    this->SetSizer(mainsizer);
}

void TableViewDialog::OnCommand(wxCommandEvent &evt)
{
    switch (evt.GetId())
    {
    case ID_TVD_COPY:
    case ID_TVD_EXPORT:
    {
        std::stringstream ss;
        size_t nc = _grid->GetNumberCols();
        for (int i = -1; i < (int)_grid->GetNumberRows(); i++)
        {
            for (size_t j = 0; j < nc; j++)
            {
                if (i < 0)
                    ss << _grid->GetColLabelValue(j);
                else
                    ss << _grid->GetCellValue(i, j);
                
                if (j < nc - 1)
                    ss << "\t";
            }
            ss << "\n";
        }
        if (evt.GetId() == ID_TVD_COPY)
        {
            if (wxTheClipboard->Open())
            {
                wxTheClipboard->Clear();
                wxTheClipboard->AddData(new wxTextDataObject( ss.str() ));
                wxTheClipboard->Close();
            }
            else
            {
                wxMessageBox("Unable to access the clipboard. Copy failed.");
            }
        }
        else
        {
            wxFileDialog dlg(this, "Export grid data", wxEmptyString, _vardata->name+".txt", wxFileSelectorDefaultWildcardStr, wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
            if (dlg.ShowModal() == wxID_OK)
            {
                wxString path = dlg.GetPath();
                FILE* f = fopen(path.ToStdString().c_str(), "w");
                if (f)
                {
                    fprintf(f, "%s", ss.str().c_str());
                    fclose(f);
                }
                else
                {
                    wxMessageBox("Specified path does not exist or file is write-protected. Data not saved.");
                }
            }
        }
        break;
    }
    case ID_TVD_GRID:
    default:
        break;
    }
    return;
}

void TableViewDialog::OnCopy(wxKeyEvent& evt)
{
    if (! (evt.GetKeyCode() == (int)'c' || evt.GetKeyCode() == (int)'C'))
    {
        evt.Skip();
        return;
    }
    if (!evt.ControlDown())
    {
        evt.Skip();
        return;
    }

    wxArrayInt selcols = _grid->GetSelectedCols();
    wxArrayInt selrows = _grid->GetSelectedRows();
    wxGridCellCoordsArray selcells = _grid->GetSelectedCells();

    std::stringstream data;
    if (selcols.GetCount() > 0)
    {
        //a set of columns has been selected
        for (int i = -1; i < (int)_grid->GetNumberRows(); i++)
        {
            for (size_t j = 0; j<selcols.GetCount(); j++)
            {
                if (i < 0)
                    data << _grid->GetColLabelValue(selcols[j]) << "\t";
                else
                    data << _grid->GetCellValue(i, selcols[j]) << "\t";
            }
            data << "\n";
        }

    }
    else if (selrows.GetCount() > 0)
    {
        //a set of rows has been selected

        //header first
        for (size_t j = 0; j < _grid->GetNumberCols(); j++)
            data << _grid->GetColLabelValue(j) << "\t";
        data << "\n";

        //data
        for (size_t i = 0; i < selrows.GetCount(); i++)
        {
            for (size_t j = 0; j < _grid->GetNumberCols(); j++)
                data << _grid->GetCellValue(selrows[i], j) << "\t";
            data << "\n";
        }
    }
    else if (selcells.GetCount() > 0)
    {
        //consolidate a list of all columns and all rows in the selection(s)
        std::vector<int> cols;
        std::vector<int> rows;
        wxGridCellCoordsArray sel_tl = _grid->GetSelectionBlockTopLeft();
        wxGridCellCoordsArray sel_br = _grid->GetSelectionBlockBottomRight();

        for (size_t s = 0; s < sel_tl.Count(); s++)
        {
            for (size_t j = sel_tl[s].GetCol(); j < sel_br[s].GetCol() + 1; j++)
                cols.push_back(j);
            for (size_t i = sel_tl[s].GetRow(); i < sel_br[s].GetRow() + 1; i++)
                rows.push_back(i);
        }

        for (int i = -1; i < (int)rows.size(); i++)
        {
            for (size_t j = 0; j < cols.size(); j++)
            {
                if (i < 0)
                    data << _grid->GetColLabelValue(cols[j]) << "\t";
                else
                    data << _grid->GetCellValue(rows[i], cols[j]) << "\t";
                data << "\n";
            }
        }
    }

    if (wxTheClipboard->Open())
    {
        wxTheClipboard->Clear();
        wxTheClipboard->AddData(new wxTextDataObject(data.str()));
        wxTheClipboard->Close();
    }
    else
        wxMessageBox("Unable to open the clipboard. Copy failed.");

    return;

}


BEGIN_EVENT_TABLE(TableViewDialog, wxDialog)
EVT_BUTTON(ID_TVD_COPY, TableViewDialog::OnCommand)
EVT_BUTTON(ID_TVD_EXPORT, TableViewDialog::OnCommand)
END_EVENT_TABLE()