#pragma once

#define FAT16_STRING	L"fat16"

class CStCmdLineProcessor : public CStBaseToCmdLineProcessor
{
public:
	CStCmdLineProcessor(CStConfigInfo* _p_config_info, CString _cmd_line);
	~CStCmdLineProcessor(void);

	BOOL AutoStart(){ return m_auto_start; }
	BOOL DefaultAction(){ return m_default_action; }
	BOOL AutoQuit(){ return m_auto_quit; }
	BOOL FormatDataArea(){ return m_format_data_area; }
    BOOL SetFormatDataArea(BOOL _format){ BOOL old_format = m_format_data_area; m_format_data_area = _format; return old_format; }
	BOOL EraseMedia(){ return m_erase_media; }
	BOOL Log(){ return m_log; }
	BOOL ForceEnglish() { return m_force_english; }
	LANGID ForceFWLanguage() { return m_force_fw_language; }
	BOOL EnableDetailsButton(){ return m_display_details_option; }
	BOOL WantedCommandLineHelp(){ return m_help_command_line; }
    BOOL Recover2DD() { return m_recover2DD; }
	BOOL StandardDialog() { return m_standard_dialog; }
    BOOL AdvancedDialog() { return m_advanced_dialog; }
	BOOL MinimalDialog() { return m_minimal_dialog; }
	virtual BOOL Fat16(){ return m_fat16; }
	BOOL SilentOperation() { return m_silent_operation; }

#ifdef _DEBUG
	virtual BOOL GenXRFiles() { return m_gen_xr_files; } //system drives read from the media
#endif

	CString GetLogFilename(){ return m_log_filename; }	

	CString GetHelpText();

private:

	ST_ERROR	ProcessCmdLine(CString _cmdline);
	void		InitToDefaults();
    ST_ERROR	SetROMOption (CStringArray& _arr_tokens, CString _cmd_line);		
	ST_ERROR	SetAutoStartOptionFromCmdLine(CStringArray& _arr_tokens);		
	ST_ERROR	SetAutoQuitOptionFromCmdLine(CStringArray& _arr_tokens);		
	ST_ERROR	SetDefaultActionOptionFromCmdLine(CStringArray& _arr_tokens);		
	ST_ERROR	SetLogOptionFromCmdLine(CStringArray& _arr_tokens, CString _cmd_line);		
	ST_ERROR	SetHelpOptionFromCmdLine(CStringArray& _arr_tokens);		
	ST_ERROR	SetFormatOptionFromCmdLine(CStringArray& _arr_tokens);		
	ST_ERROR	SetRecover2DDOptionFromCmdLine(CStringArray& _arr_tokens, CString _cmd_line);
	ST_ERROR	SetFat16OptionFromCmdLine(CStringArray& _arr_tokens);
	ST_ERROR	SetEraseMediaOptionFromCmdLine(CStringArray& _arr_tokens);		
	ST_ERROR	SetStandardDialogOptionFromCmdLine(CStringArray& _arr_tokens);		
	ST_ERROR	SetAdvancedDialogOptionFromCmdLine(CStringArray& _arr_tokens);		
	ST_ERROR	SetMinimalDialogOptionFromCmdLine(CStringArray& _arr_tokens);		
	ST_ERROR	SetForceFWLanguageFromCmdLine(CStringArray& _arr_tokens, CString _cmd_line);
	ST_ERROR	SetSilentOperationOptionFromCmdLine(CStringArray& _arr_tokens);		

	BOOL		FindOption(CStringArray& _arr_tokens, CString _token, BOOL _command=TRUE);
	
	
#ifdef _DEBUG
	ST_ERROR	SetGenXROptionFromCmdLine(CStringArray& _arr_tokens);		
#endif

	BOOL		m_default_action;
	BOOL		m_auto_start;
	BOOL		m_auto_quit;
	BOOL		m_format_data_area;
	BOOL		m_remove_drm_data;
	BOOL		m_log;
	BOOL		m_display_details_option;
	BOOL		m_help_command_line;
	BOOL		m_force_english;
	BOOL		m_fat16;
    BOOL        m_recover2DD;
    BOOL        m_recover2DDImage;
	BOOL		m_standard_dialog;
    BOOL        m_advanced_dialog;
	BOOL		m_minimal_dialog;
	LANGID		m_force_fw_language;
	BOOL		m_silent_operation;
	BOOL		m_erase_media;

#ifdef _DEBUG
	BOOL		m_gen_xr_files;
#endif

	CString		m_log_filename;

	CString		m_str_log;
	CString		m_str_auto_start;
	CString		m_str_auto_quit;
	CString		m_str_auto_all;
	CString		m_str_default_action;
	CString		m_str_format;
	CString		m_str_help;
	CString		m_str_force_english;
	CString		m_str_fat16;
    CString     m_str_recover2DD;
	CString		m_recover2dd_image_filename;
	CString		m_str_standard_dialog;
    CString     m_str_advanced_dialog;
	CString		m_str_minimal_dialog;
	CString		m_str_force_fw_language;
	CString		m_str_silent_operation;
	CString		m_str_erase_media;

#ifdef _DEBUG
	CString		m_str_gen_xr;
#endif

	CStConfigInfo*	m_p_config_info;
};
