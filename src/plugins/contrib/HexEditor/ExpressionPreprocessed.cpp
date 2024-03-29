/*
* This file is part of HexEditor plugin for Code::Blocks Studio
* Copyright (C) 2008 Bartlomiej Swiecki
*
* HexEditor plugin is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* HexEditor pluging is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with wxSmith. If not, see <http://www.gnu.org/licenses/>.
*
* $Revision: 8566 $
* $Id: ExpressionPreprocessed.cpp 8566 2012-11-14 23:39:16Z thomasdenk $
* $HeadURL: svn://svn.code.sf.net/p/codeblocks/code/branches/release-20.xx/src/plugins/contrib/HexEditor/ExpressionPreprocessed.cpp $
*/

#include "ExpressionPreprocessed.h"

namespace Expression
{
//    Value::Value( const Value& copyFrom )
//    {
//        *this = copyFrom;
//    }
//
//    // TODO: is there any other way to make sure that all members of union are copied correctly ?
//    Value& Value::operator=( const Value& copyFrom )
//    {
//        m_Type        = copyFrom.m_Type;
//        m_SignedInt   = copyFrom.m_SignedInt;
//        m_UnsignedInt = copyFrom.m_UnsignedInt;
//        m_Float       = copyFrom.m_Float;
//        return *this;
//    }

    bool Value::operator< ( const Value& second ) const
    {
        if ( m_Type != second.m_Type )
        {
            return m_Type < second.m_Type;
        }

        switch ( m_Type )
        {
            case tSignedInt:   return m_SignedInt   < second.m_SignedInt;
            case tUnsignedInt: return m_UnsignedInt < second.m_UnsignedInt;
            case tFloat:       return m_Float       < second.m_Float;
			default:break;
        }

        assert( false );
        return true;
    }

    Preprocessed::Preprocessed()
    {
    }

    Preprocessed::~Preprocessed()
    {
    }

    inline static const wxChar* Type( unsigned mod )
    {
        switch ( mod )
        {
            case Operation::modNone:           return _T("none");
            case Operation::modArg:            return _T("arg");
            case Operation::modChar:           return _T("char");
            case Operation::modByte:           return _T("byte");
            case Operation::modShort:          return _T("short");
            case Operation::modWord:           return _T("word");
            case Operation::modLong:           return _T("long");
            case Operation::modDword:          return _T("dword");
            case Operation::modLongLong:       return _T("long long");
            case Operation::modQword:          return _T("qword");
            case Operation::modFloat:          return _T("float");
            case Operation::modDouble:         return _T("double");
            case Operation::modLongDouble:     return _T("long double");
            default:                           return _T("???");
        }
    }

    wxString Preprocessed::DumpCode()
    {
        wxString res;
        for ( int i=0; i<(int)m_Code.size(); ++i )
        {
            switch ( m_Code[i].m_OpCode )
            {
                case Operation::endScript:     res += wxString::Format( _T("%d: End\n"), (int)i ); break;
                case Operation::pushCurrent:   res += wxString::Format( _T("%d: PushCur\n"), (int)i ); break;
                case Operation::loadMem:       res += wxString::Format( _T("%d: LoadMem %s, + %d\n"), (int)i, Type( m_Code[i].m_Mod1 ), (int)m_Code[i].m_ConstArgument ); break;
                case Operation::loadArg:       res += wxString::Format( _T("%d: LoadArg %s, %d\n"), (int)i, Type( m_Code[i].m_Mod1 ), (int)m_Code[i].m_ConstArgument ); break;
                case Operation::conv:          res += wxString::Format( _T("%d: Conv    %s -> %s\n"), (int)i, Type( m_Code[i].m_Mod2), Type( m_Code[i].m_Mod1) ); break;
                case Operation::add:           res += wxString::Format( _T("%d: Add\n"), (int)i ); break;
                case Operation::mul:           res += wxString::Format( _T("%d: Mul\n"), (int)i ); break;
                case Operation::div:           res += wxString::Format( _T("%d: Div\n"), (int)i ); break;
                case Operation::mod:           res += wxString::Format( _T("%d: Mod\n"), (int)i ); break;
                case Operation::neg:           res += wxString::Format( _T("%d: Neg\n"), (int)i ); break;
                case Operation::fnSin:         res += wxString::Format( _T("%d: Sin\n"), (int)i ); break;
                case Operation::fnCos:         res += wxString::Format( _T("%d: Cos\n"), (int)i ); break;
                case Operation::fnTan:         res += wxString::Format( _T("%d: Tan\n"), (int)i ); break;
                case Operation::fnLn:          res += wxString::Format( _T("%d: Ln\n"),  (int)i ); break;
                case Operation::fnPow:         res += wxString::Format( _T("%d: Pow\n"), (int)i ); break;
                default:                       res += wxString::Format( _T("%d: ???\n"), (int)i ); break;
            }
        }
        return res;
    }

    wxString Preprocessed::DumpArgs()
    {
        wxString res;
        for ( int i=0; i<(int)m_CodeArguments.size(); ++i )
        {
            Value& v = m_CodeArguments[i];
            if ( v.IsFloat() )
            {
                res += wxString::Format( _T("%d -> Float: %f\n"), (int)i, (double)v.GetFloat() );
            }
            else if ( v.IsSignedInt() )
            {
                res += wxString::Format( _T("%d -> SInt: %lld\n"), (int)i, v.GetSignedInt() );
            }
            else if ( v.IsUnsignedInt() )
            {
                res += wxString::Format( _T("%d -> UInt: %llu\n"), (int)i, v.GetUnsignedInt() );
            }
            else
            {
                res += wxString::Format( _T("%d -> Error"), (int)i );
            }
        }
        return res;
    }

}
