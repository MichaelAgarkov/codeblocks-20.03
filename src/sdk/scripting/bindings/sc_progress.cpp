/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU Lesser General Public License, version 3
 * http://www.gnu.org/licenses/lgpl-3.0.html
 *
 * $Revision: 11399 $
 * $Id: sc_progress.cpp 11399 2018-05-08 21:54:03Z fuscated $
 * $HeadURL: svn://svn.code.sf.net/p/codeblocks/code/branches/release-20.xx/src/sdk/scripting/bindings/sc_progress.cpp $
 */

#include <sdk_precomp.h>

#ifndef CB_PRECOMP
    #include <globals.h>
    #include <cbexception.h>
    #include <wx/string.h>
#endif

#include <wx/progdlg.h>

#include "sc_base_types.h"

class ProgressDialog : public wxProgressDialog
{
    public:
        ProgressDialog()
            : wxProgressDialog(_("Progress"),
                                _("Please wait while operation is in progress..."),
                                100, nullptr,
                                wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT)
        {
        }

        ~ProgressDialog() override
        {
        }

        ProgressDialog& operator=(const ProgressDialog&)
        {
            cbThrow(_T("ProgressDialog copy constructor should never be called!"));
        }

        bool DoUpdate(int value, const wxString& newmsg)
        {
            return wxProgressDialog::Update(value, newmsg, nullptr);
        }
};

DECLARE_INSTANCE_TYPE(ProgressDialog);

namespace ScriptBindings
{
    void Register_ProgressDialog()
    {
        SqPlus::SQClassDef<ProgressDialog>("ProgressDialog").
                emptyCtor().
                func(&ProgressDialog::DoUpdate, "DoUpdate");
    }
} // namespace ScriptBindings
