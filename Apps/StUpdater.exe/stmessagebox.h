/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

// CStMessageBox dialog

class CStIntArray : public CStArray<int> {
public:
	CStIntArray(size_t size):CStArray<int>(size){}
	~CStIntArray(){}
};

class CStMessageBox;

class CStLine {
public:
	CStLine( CString _line );	
	~CStLine();
	size_t GetNumTabs();
	int GetTabPosFor( int _tab_num );
	SIZE GetSize( CStMessageBox* _dlg, int _x_res, int _y_res, int  _offset );

	void Initialize();
	CString m_line;
	CStIntArray * m_arr_tabpositions;
	SIZE m_size;
}; 

class CStLinePtrArray : public CStArray<class CStLine*> {
public:
	CStLinePtrArray(CString _text, size_t size);
	~CStLinePtrArray();

	SIZE GetSize( CStMessageBox* _dlg, int _x_res, int _y_res, int  _offset );
};

class CStText {
public:
	CStText( CString _text, int _x_resolution, int _y_resolution, CStMessageBox* _dlg, int _offset );
	~CStText();
	SIZE GetSize(){ return m_size; }
	void Initialize(CString _text);
private:
	CStLinePtrArray* m_arr_lines;
	SIZE m_size;
	RECT m_rect_client;
	RECT m_rect_screen;
	int m_x_resolution;
	int m_y_resolution;
	CStMessageBox* m_dlg;
	int m_offset;
	int GetNumLinesInText( CString _text );
};

class CStMessageBox : public CDialog
{
	DECLARE_DYNAMIC(CStMessageBox)

public:
	CStMessageBox(MESSAGE_TYPE _type, CString _caption, CString _main_txt, CString _text_in_detail, CWnd* _pParent = NULL);   // standard constructor
	static int MessageBox(MESSAGE_TYPE _type, CString _caption, CString _main_txt, CString _text_in_detail, CWnd* _pParent = NULL);
	virtual ~CStMessageBox();

// Dialog Data
	enum { IDD = IDD_STMESSAGEDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedDetailsoff();
	afx_msg void OnBnClickedDetailson();
	afx_msg void OnBnClickedNo();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedYes();
	virtual BOOL OnInitDialog();
	int GetRetValue() { return m_return_value; }

protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	void PlaceYesNoDetailsBtn();
	void PlaceDetailedText();
	void PlaceOkDetailsBtn();
	void PlaceYesNoBtn();
	void PlaceMainText();
	void PlaceDivider();
	void PlaceOkBtn();
	void PlaceIcon();
	void PlaceDlg(int _num_btns_on_dlg);

	void CalculateButtonsStartingPoint( int _num_btns, int _first_btn_id1, POINT& _place);

	void HideDetailsBtn();
	void HideYesNoBtn();
	void HideOkBtn();

	void HideDetailsOnBtn();
	void HideDetailsOffBtn();
	void ShowDetailsOnBtn();
	void ShowDetailsOffBtn();

	CString m_str_main_text;
	CString m_str_detail_text;
	CString m_caption;
	MESSAGE_TYPE m_type;

	CStText *m_p_text;
	CStText *m_p_detail_text;

	int		m_x_resolution;
	int		m_y_resolution;

	RECT	m_rect_text;
	RECT	m_rect_detail_text;
	RECT	m_rect_divider;
	RECT	m_rect_ok_btn;
	RECT	m_rect_yes_btn;
	RECT	m_rect_no_btn;
	RECT	m_rect_icon;
	RECT	m_rect_dlg;
	RECT	m_rect_details_on_btn;
	RECT	m_rect_details_off_btn;
	int		m_return_value;

};
