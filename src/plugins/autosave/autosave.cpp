/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision: 11540 $
 * $Id: autosave.cpp 11540 2018-12-20 20:07:27Z fuscated $
 * $HeadURL: svn://svn.code.sf.net/p/codeblocks/code/branches/release-20.xx/src/plugins/autosave/autosave.cpp $
 */

#include "sdk.h"
#ifndef CB_PRECOMP
    #include <wx/checkbox.h>
    #include <wx/choice.h>
    #include <wx/filefn.h>
    #include <wx/filename.h>
    #include <wx/textctrl.h>
    #include <wx/timer.h>
    #include <wx/xrc/xmlres.h>
    #include "cbeditor.h"
    #include "cbproject.h"
    #include "cbworkspace.h"
    #include "configmanager.h"
    #include "editormanager.h"
    #include "globals.h"
    #include "pluginmanager.h"
    #include "projectmanager.h"
    #include "manager.h"
    #include "sdk_events.h"
    #include "workspaceloader.h"
#endif

#include <algorithm>
#include "cbstyledtextctrl.h"

#include "projectloader.h"
#include "autosave.h"

// this auto-registers the plugin
namespace
{
    PluginRegistrant<Autosave> reg(_T("Autosave"));
}

BEGIN_EVENT_TABLE(Autosave, cbPlugin)
EVT_TIMER(-1, Autosave::OnTimer)
END_EVENT_TABLE()

Autosave::Autosave()
{
    //ctor
}

Autosave::~Autosave()
{
}

void Autosave::OnAttach()
{
    if(!Manager::LoadResource(_T("autosave.zip")))
    {
        NotifyMissingFile(_T("autosave.zip"));
    }

    timer1 = new wxTimer(this, 10000);
    timer2 = new wxTimer(this, 20000);

    Start();
}

void Autosave::Start()
{
    ConfigManager *cfg = Manager::Get()->GetConfigManager(_T("autosave"));
    if(cfg->ReadBool(_T("do_project")))
        timer1->Start(60 * 1000 * cfg->ReadInt(_T("project_mins")));
    else
        timer1->Stop();

    if(cfg->ReadBool(_T("do_sources")))
        timer2->Start(60 * 1000 * cfg->ReadInt(_T("source_mins")));
    else
        timer2->Stop();
}

void Autosave::OnRelease(cb_unused bool appShutDown)
{
    delete timer1;
    delete timer2;
    timer1 = 0;
    timer2 = 0;
}

void Autosave::SaveProject(cbProject *p, int method)
{
    PluginManager *plm = Manager::Get()->GetPluginManager();
    switch(method)
    {
        case 0:
        {
            if(p->GetModified())
            {
                if(::wxRenameFile(p->GetFilename(), p->GetFilename() + _T(".bak")))
                    if(p->Save())
                    {
                        CodeBlocksEvent e(cbEVT_PROJECT_SAVE, 0, p);
                        plm->NotifyPlugins(e);
                    }
            }
            wxFileName file = p->GetFilename();
            file.SetExt(_T("layout"));
            wxString filename = file.GetFullPath();
            if(::wxRenameFile(filename, filename + _T(".bak")))
                p->SaveLayout();
            break;
        }
        case 1:
        {
            if(p->GetModified() && p->Save())
            {
                CodeBlocksEvent e(cbEVT_PROJECT_SAVE, 0, p);
                plm->NotifyPlugins(e);
            }
            p->SaveLayout();
            break;
        }
        case 2:
        case 3:
        case 4: // doesn't really make sense to keep so many versions of a project file
        {
            if (p->IsLoaded() == false)
                return;
            if(p->GetModified())
            {
                ProjectLoader loader(p);
                if(loader.Save(p->GetFilename() + _T(".save")))
                {
                    CodeBlocksEvent e(cbEVT_PROJECT_SAVE, 0, p);
                    plm->NotifyPlugins(e);
                }
                p->SetModified(); // the actual project file is still not updated!
            }
            wxFileName file = wxFileName(p->GetFilename());
            file.SetExt(_T("layout"));
            wxString filename = file.GetFullPath();
            wxString temp = filename + _T(".temp");
            wxString save = filename + _T(".save");
            if(::wxFileExists(filename) && ::wxCopyFile(filename, temp))
            {
                p->SaveLayout();
                ::wxRenameFile(filename, save);
                ::wxRenameFile(temp, filename);
            }
            break;
        }
        default:
            break;
    }
}

void Autosave::OnTimer(wxTimerEvent& e)
{
    if(e.GetId() == 10000)
    {
        ConfigManager *cfg = Manager::Get()->GetConfigManager(wxT("autosave"));

        const int method = cfg->ReadInt(_T("method"));
        const bool allProjects = cfg->ReadBool(_T("all_projects"), true);
        const bool doWorkspace = cfg->ReadBool(_T("do_workspace"), true);
        ProjectManager *pm = Manager::Get()->GetProjectManager();
        if(pm)// && pm->GetActiveProject())
        {
            if (allProjects)
            {
                ProjectsArray *projects = pm->GetProjects();
                for (size_t ii = 0; ii < projects->GetCount(); ++ii)
                    SaveProject((*projects)[ii], method);
            }
            else if(cbProject *p = pm->GetActiveProject())
                SaveProject(p, method);

            cbWorkspace *workspace = pm->GetWorkspace();
            if (doWorkspace && workspace && workspace->GetModified())
            {
                switch(method)
                {
                    case 0:
                        if(::wxRenameFile(workspace->GetFilename(), workspace->GetFilename() + _T(".bak")))
                            workspace->Save();
                        break;
                    case 1:
                        workspace->Save();
                        break;
                    case 2:
                    case 3:
                    case 4:
                    {
                        WorkspaceLoader loader;
                        loader.Save(workspace->GetTitle(), workspace->GetFilename() + wxT(".save"));
                        workspace->SetModified(true);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
    else if(e.GetId() == 20000)
    {
        int method = Manager::Get()->GetConfigManager(_T("autosave"))->ReadInt(_T("method"));
        EditorManager* em = Manager::Get()->GetEditorManager();

        if(em)
        {
            ConfigManager *cfg = Manager::Get()->GetConfigManager(wxT("app"));
            const bool robustSave = cfg->ReadBool(wxT("/environment/robust_save"), true);

            for(int i = 0; i < em->GetEditorsCount(); ++i)
            {
                cbEditor* ed = em->GetBuiltinEditor(em->GetEditor(i));
                if(ed && ed->GetModified())
                {
                    wxFileName fn(ed->GetFilename());
                    switch(method)
                    {
                        case 0:
                        {
                            if(::wxRenameFile(fn.GetFullPath(), fn.GetFullPath() + _T(".bak")))
                                cbSaveToFile(fn.GetFullPath(), ed->GetControl()->GetText(),
                                             ed->GetEncoding(), ed->GetUseBom(), robustSave);
                            break;
                        }
                        case 1:
                        {
                            ed->Save();
                            break;
                        }
                        case 2:
                        {
                            cbSaveToFile(fn.GetFullPath() + _T(".save"),
                                         ed->GetControl()->GetText(), ed->GetEncoding(),
                                         ed->GetUseBom(), robustSave);
                            ed->SetModified(); // the "real" file has not been saved!
                            break;
                        }
                        case 3:
                        case 4:
                        {
                            wxString tmp1;
                            wxString tmp2;
                            wxString backupDir = fn.GetPath();
                            if (method==4)
                            {
                                backupDir += wxT("/.bak");
                                if (!wxDirExists(backupDir) && !wxMkdir(backupDir))
                                {
                                    // Fallback if the backup directory cannot be created.
                                    backupDir = fn.GetPath();
                                }
                            }

                            for (unsigned int revisions = 8; revisions; --revisions)
                            {
                                tmp1.Printf(_T("%s/%s.%u.%s"), backupDir.c_str(), fn.GetName().c_str(), revisions,   fn.GetExt().c_str());
                                tmp2.Printf(_T("%s/%s.%u.%s"), backupDir.c_str(), fn.GetName().c_str(), revisions+1, fn.GetExt().c_str());

                                if(::wxFileExists(tmp2))
                                    ::wxRemoveFile(tmp2);
                                if(::wxFileExists(tmp1))
                                    ::wxRenameFile(tmp1, tmp2);
                            }

                            tmp1.Printf(_T("%s/%s.1.%s"), backupDir.c_str(), fn.GetName().c_str(), fn.GetExt().c_str());

                            cbSaveToFile(tmp1, ed->GetControl()->GetText(), ed->GetEncoding(),
                                         ed->GetUseBom(), robustSave);
                            ed->SetModified(); // the "real" file has not been saved!
                            break;
                        }
                        default:
                            break;
                    }
                }

            }
        }
    }

}

cbConfigurationPanel* Autosave::GetConfigurationPanel(wxWindow* parent)
{
    AutosaveConfigDlg* dlg = new AutosaveConfigDlg(parent, this);
    // deleted by the caller

    return dlg;
}



BEGIN_EVENT_TABLE(AutosaveConfigDlg, cbConfigurationPanel)
    EVT_CHECKBOX(XRCID("do_project"), AutosaveConfigDlg::OnProjectsChecked)
    EVT_CHECKBOX(XRCID("do_sources"), AutosaveConfigDlg::OnSourceChecked)
END_EVENT_TABLE()

AutosaveConfigDlg::AutosaveConfigDlg(wxWindow* parent, Autosave* plug) : plugin(plug)
{
    wxXmlResource::Get()->LoadPanel(this, parent, _T("dlgAutosave"));

    LoadSettings();
}

void AutosaveConfigDlg::LoadSettings()
{
    ConfigManager *cfg = Manager::Get()->GetConfigManager(_T("autosave"));

    bool doProjects = cfg->ReadBool(_T("do_project"));
    bool doSources = cfg->ReadBool(_T("do_sources"));
    XRCCTRL(*this, "do_project", wxCheckBox)->SetValue(doProjects);
    XRCCTRL(*this, "do_sources", wxCheckBox)->SetValue(doSources);
    XRCCTRL(*this, "do_workspace", wxCheckBox)->SetValue(cfg->ReadBool(_T("do_workspace"), true));
    XRCCTRL(*this, "all_projects", wxCheckBox)->SetValue(cfg->ReadBool(_T("all_projects"), true));
    int minsProjects = std::max(cfg->ReadInt(_T("project_mins"), 1), 1);
    int minsSources = std::max(cfg->ReadInt(_T("source_mins"), 1), 1);
    XRCCTRL(*this, "project_mins", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), minsProjects));
    XRCCTRL(*this, "source_mins", wxTextCtrl)->SetValue(wxString::Format(_T("%d"), minsSources));

    XRCCTRL(*this, "do_workspace", wxCheckBox)->Enable(doProjects);
    XRCCTRL(*this, "all_projects", wxCheckBox)->Enable(doProjects);
    XRCCTRL(*this, "project_mins", wxTextCtrl)->Enable(doProjects);

    XRCCTRL(*this, "source_mins", wxTextCtrl)->Enable(doSources);

    XRCCTRL(*this, "method", wxChoice)->SetSelection(cfg->ReadInt(_T("method"), 2));
}

void AutosaveConfigDlg::SaveSettings()
{
    ConfigManager *cfg = Manager::Get()->GetConfigManager(_T("autosave"));

    cfg->Write(_T("do_project"), (bool) XRCCTRL(*this, "do_project", wxCheckBox)->GetValue());
    cfg->Write(_T("do_sources"), (bool) XRCCTRL(*this, "do_sources", wxCheckBox)->GetValue());
    cfg->Write(_T("do_workspace"), (bool) XRCCTRL(*this, "do_workspace", wxCheckBox)->GetValue());
    cfg->Write(_T("all_projects"), (bool) XRCCTRL(*this, "all_projects", wxCheckBox)->GetValue());

    long pm, sm;

    XRCCTRL(*this, "project_mins", wxTextCtrl)->GetValue().ToLong(&pm);
    XRCCTRL(*this, "source_mins", wxTextCtrl)->GetValue().ToLong(&sm);

    if (pm < 1)
        pm = 1;
    if (sm < 1)
        sm = 1;

    cfg->Write(_T("project_mins"), (int) pm);
    cfg->Write(_T("source_mins"), (int) sm);

    cfg->Write(_T("method"), XRCCTRL(*this, "method", wxChoice)->GetSelection());

    plugin->Start();
}

void AutosaveConfigDlg::OnProjectsChecked(wxCommandEvent &event)
{
    XRCCTRL(*this, "project_mins", wxTextCtrl)->Enable(event.IsChecked());
    XRCCTRL(*this, "do_workspace", wxCheckBox)->Enable(event.IsChecked());
    XRCCTRL(*this, "all_projects", wxCheckBox)->Enable(event.IsChecked());
}

void AutosaveConfigDlg::OnSourceChecked(wxCommandEvent &event)
{
    XRCCTRL(*this, "source_mins", wxTextCtrl)->Enable(event.IsChecked());
}

