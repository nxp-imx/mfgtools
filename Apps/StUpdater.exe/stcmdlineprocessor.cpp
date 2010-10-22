/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "ddildl_defs.h"
#include "StConfigInfo.h"
#include "StProgress.h"
#include "StVersionInfo.h"
#include "StUpdater.h"
#include "StResource.h"
#include "StLogger.h"
#include ".\stcmdlineprocessor.h"
#include "StUpdaterApp.h"

extern USHORT forcedROMId;

CStCmdLineProcessor::CStCmdLineProcessor(CStConfigInfo* _p_config_info, CString _cmd_line)
{
	m_p_config_info = _p_config_info;
	InitToDefaults();
	m_last_error = ProcessCmdLine(_cmd_line);
}

CStCmdLineProcessor::~CStCmdLineProcessor(void)
{
}

ST_ERROR CStCmdLineProcessor::ProcessCmdLine(CString _cmd_line)
{
	ST_ERROR err = STERR_NONE;
	CString res_token;
	int cur_pos = 0;
	CStringArray arr_tokens;

	res_token = _cmd_line.Tokenize(CString(" "), cur_pos);
	while (res_token != "")
	{
		res_token.Trim(CString(" "));
		res_token.MakeLower();
		arr_tokens.Add( res_token );
		//printf("Resulting token: %s\n", res_token);
		res_token= _cmd_line.Tokenize(CString(" "), cur_pos);
	};
	
	err = SetAutoStartOptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

	err = SetAutoQuitOptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

	err = SetDefaultActionOptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

	err = SetLogOptionFromCmdLine(arr_tokens, _cmd_line);		
	if( err != STERR_NONE )
		return err;

	err = SetHelpOptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

	err = SetFormatOptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

	err = SetFat16OptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

	err = SetRecover2DDOptionFromCmdLine(arr_tokens, _cmd_line);		
	if( err != STERR_NONE )
		return err;

#ifdef _DEBUG
	err = SetGenXROptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;
#endif

	err = SetEraseMediaOptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

	err = SetStandardDialogOptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

	err = SetAdvancedDialogOptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

	err = SetMinimalDialogOptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

	if ( ( m_minimal_dialog && ( m_standard_dialog || m_advanced_dialog ) ) ||
		 ( m_standard_dialog && ( m_minimal_dialog || m_advanced_dialog ) ) ||
		 ( m_advanced_dialog && ( m_standard_dialog || m_minimal_dialog ) ) )
		return STERR_INVALID_REQUEST;

	err = SetForceFWLanguageFromCmdLine(arr_tokens, _cmd_line);
	if( err != STERR_NONE )
		return err;

	err = SetSilentOperationOptionFromCmdLine(arr_tokens);		
	if( err != STERR_NONE )
		return err;

    err = SetROMOption(arr_tokens, _cmd_line);

	// don't allow silent operation without auto start/quit and default action
	if ( (!m_auto_start || !m_auto_quit || !m_default_action) && m_silent_operation )
		m_auto_start = m_auto_quit = m_default_action = TRUE;  

	return err;
}

ST_ERROR CStCmdLineProcessor::SetROMOption(CStringArray& _arr_tokens, CString _cmd_line)
{
    CString ROMRev = "";
    CString OpStr = "romid";
	int from_pos=0;
	if( FindOption(_arr_tokens, OpStr) )
	{
		// now extract the ROM ID from command line.
		_cmd_line.MakeLower();
		if( (from_pos = _cmd_line.Find(OpStr)) != -1 )
		{
			from_pos += 5+1;
			ROMRev = _cmd_line.Tokenize( CString(" "), from_pos );	
            swscanf_s (ROMRev, _T("%d"), &forcedROMId);
        }
    }

    return STERR_NONE;
}

void CStCmdLineProcessor::InitToDefaults()
{
	m_default_action = FALSE;
	m_auto_start = m_p_config_info->GetDefaultAutoStartOption();
	m_auto_quit = m_p_config_info->GetDefaultAutoCloseOption();
	m_format_data_area = FALSE;
	m_log = FALSE;
	m_display_details_option = TRUE;
	m_log_filename = "";
	m_help_command_line = FALSE;
	m_fat16	= FALSE;
    m_recover2DD = FALSE;
    m_recover2DDImage = FALSE;
    m_advanced_dialog = FALSE;
	m_standard_dialog = FALSE;
	m_minimal_dialog = FALSE;
	m_force_fw_language = 0;
	m_erase_media = FALSE;

#ifdef _DEBUG
	m_gen_xr_files = FALSE;
#endif

	m_str_auto_start		= "autostart";
	m_str_auto_quit			= "autoquit";
	m_str_auto_all			= "autoall";
	m_str_default_action	= "default";
	m_str_log				= "log";
	m_str_format			= "format";
	m_str_help				= "?";
	m_str_force_english		= "english";
	m_str_fat16				= FAT16_STRING;
    m_str_recover2DD		= "recover2dd";
	m_str_standard_dialog	= "std";
    m_str_advanced_dialog	= "adv";
	m_str_minimal_dialog	= "min";
	m_str_force_fw_language = "lang";
	m_str_silent_operation	= "silent";
	m_str_erase_media		= "erasemedia";
	
#ifdef _DEBUG
	m_str_gen_xr = "XR";
	m_str_gen_xr.MakeLower();
#endif
}

ST_ERROR CStCmdLineProcessor::SetAutoStartOptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_auto_start = FALSE;
	if( FindOption(_arr_tokens, m_str_auto_start) )
	{
		m_auto_start = TRUE;
	}
	if( !m_auto_start )
	{
		if( FindOption(_arr_tokens, m_str_auto_all) )
		{
			m_auto_start = TRUE;
		}
	}

	return err;
}

ST_ERROR CStCmdLineProcessor::SetAutoQuitOptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_auto_quit = FALSE;
	if( FindOption(_arr_tokens, m_str_auto_quit) )
	{
		m_auto_quit = TRUE;
	}
	if( m_auto_quit == FALSE )
	{
		if( FindOption(_arr_tokens, m_str_auto_all) )
		{
			m_auto_quit = TRUE;
		}
	}

	return err;
}		

ST_ERROR CStCmdLineProcessor::SetDefaultActionOptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_default_action = FALSE;
	if( FindOption(_arr_tokens, m_str_default_action) )
	{
		m_default_action = TRUE;
	}
	if( m_default_action == FALSE )
	{
		if( FindOption(_arr_tokens, m_str_auto_all) )
		{
			m_default_action = TRUE;
		}
	}
	return err;
}

ST_ERROR CStCmdLineProcessor::SetLogOptionFromCmdLine(CStringArray& _arr_tokens, CString _cmd_line)
{
	ST_ERROR err = STERR_NONE;
	int from_pos=0;

	m_log = FALSE;
	m_force_english = FALSE;
	if( FindOption(_arr_tokens, m_str_log) )
	{
		m_log = TRUE;
	}

	if( m_log == TRUE )
	{
		if( FindOption(_arr_tokens, m_str_force_english, FALSE) )
		{
			m_force_english = TRUE;
		}
	}

	if( m_log )
	{
		// now extract the filename from command line.
		_cmd_line.MakeLower();
		m_log_filename = "";
		if( (from_pos = _cmd_line.Find(m_str_log)) != -1 )
		{
			from_pos += m_str_log.GetLength()+1;
			m_log_filename = _cmd_line.Tokenize( CString(" "), from_pos );	
			if( m_force_english )
			{
				if( m_log_filename.CompareNoCase( m_str_force_english ) == 0 )
				{
					if( (from_pos = _cmd_line.Find(m_str_log, from_pos)) != -1 )
					{
						from_pos += m_str_log.GetLength()+1;
						m_log_filename = _cmd_line.Tokenize( CString(" "), from_pos );	
					}
				}
			}
		}

//		if( m_log_filename == "" )
//		{
//			return STERR_MISSING_CMDLINE_PARAMETER_FILENAME;
//		}
	}
	return err;
}

ST_ERROR CStCmdLineProcessor::SetHelpOptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_help_command_line = FALSE;
	if( FindOption(_arr_tokens, m_str_help) )
	{
		m_help_command_line = TRUE;
	}

	return err;
}

ST_ERROR CStCmdLineProcessor::SetFormatOptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_format_data_area = FALSE;
	if( FindOption(_arr_tokens, m_str_format) )
	{
		m_format_data_area = TRUE;
	}
	return err;
}

ST_ERROR CStCmdLineProcessor::SetEraseMediaOptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_erase_media = FALSE;
	if( FindOption(_arr_tokens, m_str_erase_media) )
	{
		m_erase_media = TRUE;
	}
	return err;
}

ST_ERROR CStCmdLineProcessor::SetFat16OptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_fat16 = FALSE;
	if( FindOption(_arr_tokens, m_str_fat16) )
	{
		m_fat16 = TRUE;
	}
	return err;
}

ST_ERROR CStCmdLineProcessor::SetRecover2DDOptionFromCmdLine(CStringArray& _arr_tokens, CString _cmd_line)
{
	ST_ERROR err = STERR_NONE;
	int from_pos=0;
    CString imgFile;

	m_recover2DD = FALSE;
	if( FindOption(_arr_tokens, m_str_recover2DD) )
	{
		m_recover2DD = TRUE;
        m_format_data_area = TRUE; // force the format option
        m_p_config_info->SetRecover2DD(TRUE);

        m_recover2DDImage = FALSE;

		// now extract the filename from command line.
		_cmd_line.MakeLower();
		imgFile = "";
		if( (from_pos = _cmd_line.Find(m_str_recover2DD)) != -1 )
		{
			from_pos += m_str_recover2DD.GetLength()+1;
//            m_recover2dd_image_filename = _cmd_line.Tokenize( CString(" "), from_pos );	
            imgFile = _cmd_line.Tokenize( CString(" "), from_pos );	
		}

		if( imgFile != "" )
		{
            wstring wtp;
            wtp = imgFile;
            m_recover2DDImage = TRUE;
            m_p_config_info->SetRecover2DDImage(TRUE);
            m_p_config_info->SetRecover2DDImageFileName(wtp);
		}
	}
	return err;
}

ST_ERROR CStCmdLineProcessor::SetStandardDialogOptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_standard_dialog = FALSE;
	if( FindOption(_arr_tokens, m_str_standard_dialog) )
	{
		m_standard_dialog = TRUE;
	}
	return err;
}

ST_ERROR CStCmdLineProcessor::SetAdvancedDialogOptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_advanced_dialog = FALSE;
	if( FindOption(_arr_tokens, m_str_advanced_dialog) )
	{
		m_advanced_dialog = TRUE;
	}
	return err;
}

ST_ERROR CStCmdLineProcessor::SetMinimalDialogOptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_minimal_dialog = FALSE;
	if( FindOption(_arr_tokens, m_str_minimal_dialog) )
	{
		m_minimal_dialog = TRUE;
	}
	return err;
}

//#define _CRT_SECURE_NO_DEPRECATE 1
ST_ERROR CStCmdLineProcessor::SetForceFWLanguageFromCmdLine(CStringArray& _arr_tokens, CString _cmd_line)
{
	ST_ERROR err = STERR_NONE;

	if( FindOption(_arr_tokens, m_str_force_fw_language) )
	{
		int from_pos=0;
		CString szLangId;
		WORD wPrimaryId, wSubId;

		if( (from_pos = _cmd_line.Find(m_str_force_fw_language)) != -1 )
		{
			from_pos += m_str_force_fw_language.GetLength()+1;
            szLangId = _cmd_line.Tokenize( CString(" "), from_pos );

			_stscanf_s ((LPCTSTR)szLangId.GetString(), L"%x,&x", &wPrimaryId, &wSubId);
			m_force_fw_language = MAKELANGID(wPrimaryId, wSubId);
		}
	}
	return err;
}

ST_ERROR CStCmdLineProcessor::SetSilentOperationOptionFromCmdLine(CStringArray& _arr_tokens)		
{
	ST_ERROR err = STERR_NONE;

	m_silent_operation = FALSE;
	if( FindOption(_arr_tokens, m_str_silent_operation) )
	{
		m_silent_operation = TRUE;
	}
	return err;
}


#ifdef _DEBUG
ST_ERROR CStCmdLineProcessor::SetGenXROptionFromCmdLine(CStringArray& _arr_tokens)
{
	ST_ERROR err = STERR_NONE;

	m_gen_xr_files = FALSE;
	if( FindOption(_arr_tokens, m_str_gen_xr) )
	{
		m_gen_xr_files = TRUE;
	}
	return err;
}
#endif

BOOL CStCmdLineProcessor::FindOption(CStringArray& _arr_tokens, CString _token, BOOL _command)
{
	BOOL token_found = FALSE;
	for( int index=0; index<_arr_tokens.GetCount(); index++)
	{
		if( _command )
		{
			if( _arr_tokens.GetAt(index).Find( CString("-") + _token ) != -1 )
			{
				token_found = TRUE;
				break;
			}
			if( _arr_tokens.GetAt(index).Find( CString("/") + _token ) != -1 )
			{
				token_found = TRUE;
				break;
			}
		}
		else
		{
			if( _arr_tokens.GetAt(index).Find( _token ) != -1 )
			{
				token_found = TRUE;
				break;
			}
		}
	}
	return token_found;
}

CString CStCmdLineProcessor::GetHelpText()
{
	CString help_text, help_text_format;
	wstring exe_name;

	((CStUpdaterApp*) AfxGetApp())->GetResource()->GetResourceString(IDS_CMDLINE_HELPTEXT, help_text_format);

	m_p_config_info->ExecutableName(exe_name);

	help_text.Format( help_text_format, 
		exe_name.c_str(), 
		m_str_auto_start,
		m_str_auto_quit,
		m_str_default_action,
		m_str_auto_all,
		m_str_format,
        m_str_erase_media,
		m_str_log,
		m_str_log,
		m_str_force_fw_language,
		m_str_help,
		m_str_auto_start,
		m_str_auto_quit,
		m_str_default_action,
		m_str_auto_all,
		m_str_auto_start,
		m_str_auto_quit,
		m_str_default_action,
		m_str_format,
        m_str_erase_media,
		m_str_log,
		m_str_log,
		m_str_force_fw_language,
		m_str_help
		);

	return help_text;
}
