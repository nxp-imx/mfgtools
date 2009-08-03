// StMessageBox.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "StGlobals.h"
#include ".\stmessagebox.h"
#include "windows.h"
#include "assert.h"
// CStMessageBox dialog

#define SPACE_BETWEEN_CONTROLS	15
#define SPACE_BETWEEN_BUTTONS	4

CStLine::CStLine( CString _line )
{
	m_line = _line;
	m_arr_tabpositions = NULL;
	Initialize();
}	

CStLine::~CStLine()
{
	if( m_arr_tabpositions )
		delete m_arr_tabpositions;
}

void CStLine::Initialize()
{
	CString temp("");
	int index_to_arr = 0;
	int i;
	int *arr_tab_pos = new int[m_line.GetLength()];

	for( i=0; i<m_line.GetLength(); i++ )
	{
		if( m_line.GetAt(i) == '\t' )
		{	
			arr_tab_pos[index_to_arr++] = i;
			continue;
		}
		temp += m_line.GetAt(i);
	}

	m_line = temp;
	if( index_to_arr > 0 )
	{
		m_arr_tabpositions = new CStIntArray( index_to_arr );
		for( i=0; i<index_to_arr; i++ )
		{
			m_arr_tabpositions->SetAt(i, arr_tab_pos[i]);
		}
	}
	delete [] arr_tab_pos;
}

size_t CStLine::GetNumTabs()
{
	if( m_arr_tabpositions )
		return m_arr_tabpositions->GetCount();
	return 0;
}

int CStLine::GetTabPosFor( int _tab_num )
{
	if( m_arr_tabpositions )
		return *m_arr_tabpositions->GetAt( _tab_num );
	else
	{
		assert(0);
		return 0;
	}
}

SIZE CStLine::GetSize( CStMessageBox* _dlg, int _x_res, int _y_res, int _offset )
{
	SIZE size;
	CFont* font = _dlg->GetFont();
	CDC* dc = _dlg->GetDC();
	HGDIOBJ oldfont = dc->SelectObject( font );

	if( m_arr_tabpositions )
	{
		size = dc->GetTextExtent( m_line );
		SIZE tab_size = dc->GetTabbedTextExtent( L"\t", 1, 0, NULL );

		size.cx += (LONG)( m_arr_tabpositions->GetCount() * tab_size.cx );
	}
	else
	{
		size = dc->GetTextExtent( m_line );
	}

	dc->SelectObject( oldfont );
	_dlg->ReleaseDC( dc );

	int num_lines = 0;
	_x_res /= 5;
	_x_res *= 3;
	if( size.cx > _x_res )
	{
		num_lines = size.cx / ( _x_res - ( SPACE_BETWEEN_CONTROLS * 3 ) - _offset );
		if( size.cx % ( _x_res - ( SPACE_BETWEEN_CONTROLS * 3 ) - _offset ) )
		{
			num_lines ++;
		}
		size.cx =  _x_res - ( SPACE_BETWEEN_CONTROLS * 3 ) - _offset;
		size.cy *= num_lines;	
	}

	return size;
	UNREFERENCED_PARAMETER(_y_res);
}

CStLinePtrArray::CStLinePtrArray(CString _text, size_t _size):
	CStArray<class CStLine*>(_size)
{
	int index=0;
	CString resToken("");

	for( size_t i=0; i<(size_t)_text.GetLength(); i++ )
	{
		if( (_text.GetAt((int)i) == '\n' ) || ( i == (size_t)(_text.GetLength()-1 ) )  )
		{
			resToken += _text.GetAt((int)i);
			CStLine* line = new CStLine(resToken);
			SetAt(index++, line);
			resToken = CString("");
		}
		else
		{
			resToken += _text.GetAt((int)i);
		}
	}
	
	if( index == 0 )
	{
		CStLine* line = new CStLine(_text);
		SetAt(0, line);
	}
}

CStLinePtrArray::~CStLinePtrArray()
{
	CStLine* line;
	for(size_t index=0; index<GetCount(); index ++)
	{
		line = *GetAt(index);
		delete line;
		SetAt(index, NULL);
	}
}
SIZE CStLinePtrArray::GetSize( CStMessageBox* _dlg, int _x_resolution, int _y_resolution, int _offset )
{
	CStLine* line;
	SIZE line_size, line_size_largest;
	line = *GetAt(0);
	line_size_largest = line->GetSize( _dlg, _x_resolution, _y_resolution, _offset );
	for(size_t index=1; index<GetCount(); index ++)
	{
		line = *GetAt(index);
		line_size = line->GetSize( _dlg, _x_resolution, _y_resolution, _offset );
		if( line_size.cx > line_size_largest.cx )
		{
			line_size_largest.cx = line_size.cx;
		}
		line_size_largest.cy += line_size.cy;
	}

	return line_size_largest;
}

CStText::CStText( CString _text, int _x_resolution, int _y_resolution, CStMessageBox* _dlg, int _offset )
{
	m_x_resolution = _x_resolution;
	m_y_resolution = _y_resolution;
	m_dlg = _dlg;
	m_offset = _offset;
	m_arr_lines = NULL;
	Initialize( _text );
}

CStText::~CStText()
{
	if( m_arr_lines )
		delete m_arr_lines;
}

void CStText::Initialize(CString _text)
{
	int num_lines = GetNumLinesInText( _text );
	m_arr_lines = new CStLinePtrArray( _text, num_lines );

	m_size = m_arr_lines->GetSize(m_dlg, m_x_resolution, m_y_resolution, m_offset);
}

int CStText::GetNumLinesInText( CString _text )
{
	int count=1;

	for(int i=0; i<_text.GetLength(); i++ )
	{
		if( ( _text.GetAt(i) == '\n' ) && (i != ( _text.GetLength()-1 ) ) )
		{
			count ++;
		}
	}

	return count;
}

IMPLEMENT_DYNAMIC(CStMessageBox, CDialog)
CStMessageBox::CStMessageBox(MESSAGE_TYPE _type, CString _caption, CString _main_txt, CString _text_in_detail, CWnd* pParent /*=NULL*/)
	: CDialog(CStMessageBox::IDD, pParent)
{
	m_caption = _caption;
	m_str_main_text = _main_txt;
	m_str_detail_text = _text_in_detail;
	m_type = _type;
	m_return_value = IDOK;
	m_p_text = NULL;
	m_p_detail_text = NULL;
}

CStMessageBox::~CStMessageBox()
{
	if( m_p_text ) 
		delete m_p_text;
	if( m_p_detail_text ) 
		delete m_p_detail_text;
}

void CStMessageBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStMessageBox, CDialog)
	ON_BN_CLICKED(IDDETAILSOFF, OnBnClickedDetailsoff)
	ON_BN_CLICKED(IDDETAILSON, OnBnClickedDetailson)
	ON_BN_CLICKED(IDNO, OnBnClickedNo)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDYES, OnBnClickedYes)
END_MESSAGE_MAP()


// CStMessageBox message handlers

void CStMessageBox::OnBnClickedDetailsoff()
{
	// TODO: Add your control notification handler code here
    RECT rDlg, rDivider;

    GetWindowRect(&rDlg);

	GetDlgItem( IDC_DIVIDER )->GetWindowRect( &rDivider );

	::MoveWindow(m_hWnd, rDlg.left, rDlg.top, (rDlg.right - rDlg.left), (rDivider.bottom - rDlg.top), TRUE);

	ShowDetailsOnBtn();
	HideDetailsOffBtn();
}

void CStMessageBox::OnBnClickedDetailson()
{
	// TODO: Add your control notification handler code here
    RECT rDlg, rDetails;

    GetWindowRect(&rDlg);

	GetDlgItem( IDC_DETAIL_TEXT )->GetWindowRect(&rDetails);
	::MoveWindow(m_hWnd, rDlg.left, rDlg.top, (rDlg.right - rDlg.left), (rDetails.bottom - rDlg.top) + SPACE_BETWEEN_CONTROLS, TRUE);

	ShowDetailsOffBtn();
	HideDetailsOnBtn();
}

void CStMessageBox::OnBnClickedNo()
{
	// TODO: Add your control notification handler code here
	m_return_value = IDNO;
	OnOK();
}

void CStMessageBox::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	m_return_value = IDOK;
	OnOK();
}

void CStMessageBox::OnBnClickedYes()
{
	// TODO: Add your control notification handler code here
	m_return_value = IDYES;
	OnOK();
}

BOOL CStMessageBox::OnInitDialog()
{
	HICON h_icon;
	RECT rIcon;
	int num_btns=0;
	CString str_res;
	CDialog::OnInitDialog();

	if ( str_res.LoadString( IDS_MESSAGE_BOX_OK ) )
		SetDlgItemText( IDOK, str_res );

	if ( str_res.LoadString( IDS_MESSAGE_BOX_DETAILS_OFF ) )
		SetDlgItemText( IDDETAILSON, str_res );

	if ( str_res.LoadString( IDS_MESSAGE_BOX_DETAILS_ON ) )
		SetDlgItemText( IDDETAILSOFF, str_res );

	if ( str_res.LoadString( IDS_MESSAGE_BOX_YES ) )
		SetDlgItemText( IDYES, str_res );

	if ( str_res.LoadString( IDS_MESSAGE_BOX_NO ) )
		SetDlgItemText( IDNO, str_res );

	m_x_resolution = GetSystemMetrics( SM_CXSCREEN );
	m_y_resolution = GetSystemMetrics( SM_CYSCREEN );

	GetDlgItem( IDC_ICON_TYPE )->GetWindowRect( &rIcon );

	m_p_text = new CStText( m_str_main_text, m_x_resolution, m_y_resolution, this, rIcon.right - rIcon.left );
	m_p_detail_text = new CStText( m_str_detail_text, m_x_resolution, m_y_resolution, this, 0 );

	PlaceIcon();
	PlaceMainText();
	PlaceDetailedText();
	PlaceDivider();

	switch( m_type )
	{
	case MSG_TYPE_INFO:
	//set icon
		h_icon = AfxGetApp()->LoadStandardIcon(IDI_INFORMATION);
		((CStatic*)GetDlgItem( IDC_ICON_TYPE ))->SetIcon( h_icon );
		
		HideYesNoBtn();
		
		if( m_str_detail_text.GetLength() == 0 )
		{
			HideDetailsBtn();
			PlaceOkBtn();
			num_btns = 1;
		}
		else
		{
			PlaceOkDetailsBtn();
			HideDetailsOffBtn();
			ShowDetailsOnBtn();
			num_btns = 2;
		}
		break;
	case MSG_TYPE_WARNING:
		h_icon = AfxGetApp()->LoadStandardIcon(IDI_WARNING);
		((CStatic*)GetDlgItem( IDC_ICON_TYPE ))->SetIcon( h_icon );

		HideYesNoBtn();

		if( m_str_detail_text.GetLength() == 0 )
		{
			HideDetailsBtn();
			PlaceOkBtn();
			num_btns = 1;
		}
		else
		{
			PlaceOkDetailsBtn();
			HideDetailsOffBtn();
			ShowDetailsOnBtn();
			num_btns = 2;
		}
		break;
	case MSG_TYPE_QUESTION:
		h_icon = AfxGetApp()->LoadStandardIcon(IDI_QUESTION);
		((CStatic*)GetDlgItem( IDC_ICON_TYPE ))->SetIcon( h_icon );
		
		HideOkBtn();

		if( m_str_detail_text.GetLength() == 0 )
		{
			HideDetailsBtn();
			PlaceYesNoBtn();
			num_btns = 2;
		}
		else
		{
			PlaceYesNoDetailsBtn();
			HideDetailsOffBtn();
			ShowDetailsOnBtn();
			num_btns = 3;
		}
		break;
	case MSG_TYPE_ERROR:
	case MSG_TYPE_ERROR_UNDEFINED:
	default:

		h_icon = AfxGetApp()->LoadStandardIcon(IDI_ERROR);
		((CStatic*)GetDlgItem( IDC_ICON_TYPE ))->SetIcon( h_icon );

		HideYesNoBtn();
		
		if( m_str_detail_text.GetLength() == 0 )
		{
			HideDetailsBtn();
			PlaceOkBtn();
			num_btns = 1;
		}
		else
		{
			PlaceOkDetailsBtn();
			HideDetailsOffBtn();
			ShowDetailsOnBtn();
			num_btns = 2;
		}
		break;
	}
	
	PlaceDlg(num_btns);
	SetWindowText( m_caption );
	GetDlgItem( IDC_MAIN_TEXT )->SetWindowText(m_str_main_text);
	GetDlgItem( IDC_DETAIL_TEXT )->SetWindowText(m_str_detail_text);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CStMessageBox::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// TODO: Add your specialized code here and/or call the base class

	return CDialog::OnNotify(wParam, lParam, pResult);
}
int CStMessageBox::MessageBox(MESSAGE_TYPE _type, CString _caption, CString _main_txt, CString _text_in_detail, CWnd* _pParent)
{
	CStMessageBox MsgDlg(_type, _caption, _main_txt, _text_in_detail, _pParent) ;
	MsgDlg.DoModal();
	return MsgDlg.GetRetValue();
}

void CStMessageBox::PlaceMainText()
{
	GetDlgItem( IDC_MAIN_TEXT )->GetWindowRect( &m_rect_text );
	ScreenToClient( &m_rect_text );

	m_rect_text.left = m_rect_icon.right + SPACE_BETWEEN_CONTROLS;
	m_rect_text.top = SPACE_BETWEEN_CONTROLS;

	CFont* font = GetFont();
	CDC* dc = GetDC();
	HGDIOBJ oldfont = dc->SelectObject( font );
	CSize size = dc->GetTextExtent( CString("W") );
	dc->SelectObject( oldfont );
	ReleaseDC( dc );

	if( size.cy == m_p_text->GetSize().cy )
	{
		//this is just one single line of text, hence text should be in the middle of the icon.

		m_rect_text.top = SPACE_BETWEEN_CONTROLS + 
			( ( ( m_rect_icon.bottom - m_rect_icon.top ) / 2 ) - ( size.cy / 2 ) );
	}

	::MoveWindow( GetDlgItem( IDC_MAIN_TEXT )->m_hWnd, m_rect_text.left, m_rect_text.top, m_p_text->GetSize().cx, m_p_text->GetSize().cy, TRUE );

	GetDlgItem( IDC_MAIN_TEXT )->GetWindowRect( &m_rect_text );
	ScreenToClient( &m_rect_text );
}

void CStMessageBox::PlaceDetailedText()
{
	GetDlgItem( IDC_DETAIL_TEXT )->GetWindowRect( &m_rect_detail_text );
	ScreenToClient( &m_rect_detail_text );

	m_rect_detail_text.left = SPACE_BETWEEN_CONTROLS;

	int top_offset = ( m_rect_text.bottom > m_rect_icon.bottom ) ? m_rect_text.bottom : m_rect_icon.bottom;
	
	RECT m_rect_btn;
	GetDlgItem( IDOK )->GetWindowRect( &m_rect_btn );

	m_rect_detail_text.top = top_offset + SPACE_BETWEEN_CONTROLS + ( m_rect_btn.bottom - m_rect_btn.top ) + SPACE_BETWEEN_CONTROLS + 1 + SPACE_BETWEEN_CONTROLS;

	::MoveWindow( GetDlgItem( IDC_DETAIL_TEXT )->m_hWnd, m_rect_detail_text.left, m_rect_detail_text.top, m_p_detail_text->GetSize().cx, m_p_detail_text->GetSize().cy, TRUE );

	GetDlgItem( IDC_DETAIL_TEXT )->GetWindowRect( &m_rect_detail_text );
	ScreenToClient( &m_rect_detail_text );
}

void CStMessageBox::PlaceDivider()
{
	GetDlgItem( IDC_DIVIDER )->GetWindowRect( &m_rect_divider );
	ScreenToClient( &m_rect_divider );

	m_rect_divider.left = 0;

	int top_offset = ( m_rect_text.bottom > m_rect_icon.bottom ) ? m_rect_text.bottom : m_rect_icon.bottom;
	
	RECT m_rect_btn;
	GetDlgItem( IDOK )->GetWindowRect( &m_rect_btn );

	m_rect_divider.top = top_offset + SPACE_BETWEEN_CONTROLS + ( m_rect_btn.bottom - m_rect_btn.top ) + SPACE_BETWEEN_CONTROLS;

	int right_offset = m_rect_text.right + SPACE_BETWEEN_CONTROLS;
	if( right_offset < m_rect_detail_text.right + SPACE_BETWEEN_CONTROLS )
	{
		right_offset = m_rect_detail_text.right + SPACE_BETWEEN_CONTROLS;
	}
	
	::MoveWindow( GetDlgItem( IDC_DIVIDER )->m_hWnd, m_rect_divider.left, m_rect_divider.top, 
		right_offset, 1, TRUE );

	GetDlgItem( IDC_DIVIDER )->GetWindowRect( &m_rect_divider );
	ScreenToClient( &m_rect_divider );
}

void CStMessageBox::PlaceOkBtn()
{
	POINT first_btn_place;
	CalculateButtonsStartingPoint( 1, IDOK, first_btn_place );

	GetDlgItem( IDOK )->GetWindowRect( &m_rect_ok_btn );
	ScreenToClient( &m_rect_ok_btn );

	::MoveWindow( GetDlgItem( IDOK )->m_hWnd, first_btn_place.x, first_btn_place.y, 
		( m_rect_ok_btn.right - m_rect_ok_btn.left ), ( m_rect_ok_btn.bottom - m_rect_ok_btn.top ), TRUE );

	GetDlgItem( IDOK )->GetWindowRect( &m_rect_ok_btn );
	ScreenToClient( &m_rect_ok_btn );
}

void CStMessageBox::CalculateButtonsStartingPoint( int _num_btns, int _first_btn_id1, POINT& _place)
{
	RECT r1;
	GetDlgItem( _first_btn_id1 )->GetWindowRect( &r1 );
	ScreenToClient( &r1 );

	int min_dlgwidth_required = ( ( r1.right - r1.left ) * _num_btns ) + 
		( SPACE_BETWEEN_CONTROLS * 2 ) + 
		( SPACE_BETWEEN_BUTTONS * ( _num_btns - 1 ) );
	
	if( min_dlgwidth_required < ( m_rect_text.right + SPACE_BETWEEN_CONTROLS ) )
	{
		min_dlgwidth_required = ( m_rect_text.right + SPACE_BETWEEN_CONTROLS );
	}
	
	if( min_dlgwidth_required < m_rect_detail_text.right + SPACE_BETWEEN_CONTROLS )
	{
		min_dlgwidth_required = m_rect_detail_text.right + SPACE_BETWEEN_CONTROLS;
	}

	int btns_width = ( r1.right - r1.left ) * _num_btns + (SPACE_BETWEEN_BUTTONS * ( _num_btns - 1 ) );

	_place.x = ( min_dlgwidth_required - btns_width ) / 2;

	_place.y = ( m_rect_text.bottom > m_rect_icon.bottom ) ? m_rect_text.bottom : m_rect_icon.bottom;

	_place.y += SPACE_BETWEEN_CONTROLS;
}

void CStMessageBox::PlaceYesNoBtn()
{
	POINT first_btn_place;
	CalculateButtonsStartingPoint( 2, IDYES, first_btn_place );

	GetDlgItem( IDYES )->GetWindowRect( &m_rect_yes_btn);
	ScreenToClient( &m_rect_yes_btn);
	::MoveWindow( GetDlgItem( IDYES )->m_hWnd, first_btn_place.x, first_btn_place.y, 
		( m_rect_yes_btn.right - m_rect_yes_btn.left ),
		( m_rect_yes_btn.bottom - m_rect_yes_btn.top ), TRUE );
	GetDlgItem( IDYES )->GetWindowRect( &m_rect_yes_btn);
	ScreenToClient( &m_rect_yes_btn);
	first_btn_place.x += m_rect_yes_btn.right - m_rect_yes_btn.left + SPACE_BETWEEN_BUTTONS;

	GetDlgItem( IDNO )->GetWindowRect( &m_rect_no_btn);
	ScreenToClient( &m_rect_no_btn);
	::MoveWindow( GetDlgItem( IDNO )->m_hWnd, first_btn_place.x, first_btn_place.y, 
		( m_rect_no_btn.right - m_rect_no_btn.left ),
		( m_rect_no_btn.bottom - m_rect_no_btn.top ), TRUE );
	GetDlgItem( IDNO )->GetWindowRect( &m_rect_no_btn);
	ScreenToClient( &m_rect_no_btn);
}

void CStMessageBox::PlaceYesNoDetailsBtn()
{
	POINT first_btn_place;
	CalculateButtonsStartingPoint( 2, IDYES, first_btn_place );

	GetDlgItem( IDYES )->GetWindowRect( &m_rect_yes_btn);
	ScreenToClient( &m_rect_yes_btn);
	::MoveWindow( GetDlgItem( IDYES )->m_hWnd, first_btn_place.x, first_btn_place.y, 
		( m_rect_yes_btn.right - m_rect_yes_btn.left ),
		( m_rect_yes_btn.bottom - m_rect_yes_btn.top ), TRUE );
	GetDlgItem( IDYES )->GetWindowRect( &m_rect_yes_btn);
	ScreenToClient( &m_rect_yes_btn);
	first_btn_place.x += m_rect_yes_btn.right - m_rect_yes_btn.left + SPACE_BETWEEN_BUTTONS;

	GetDlgItem( IDNO )->GetWindowRect( &m_rect_no_btn);
	ScreenToClient( &m_rect_no_btn);
	::MoveWindow( GetDlgItem( IDNO )->m_hWnd, first_btn_place.x, first_btn_place.y, 
		( m_rect_no_btn.right - m_rect_no_btn.left ),
		( m_rect_no_btn.bottom - m_rect_no_btn.top ), TRUE );
	GetDlgItem( IDNO )->GetWindowRect( &m_rect_no_btn);
	ScreenToClient( &m_rect_no_btn);
	first_btn_place.x += m_rect_no_btn.right - m_rect_no_btn.left + SPACE_BETWEEN_BUTTONS;

	GetDlgItem( IDDETAILSON )->GetWindowRect( &m_rect_details_on_btn);
	ScreenToClient( &m_rect_details_on_btn);
	::MoveWindow( GetDlgItem( IDDETAILSON )->m_hWnd, first_btn_place.x, first_btn_place.y, 
		( m_rect_details_on_btn.right - m_rect_details_on_btn.left ),
		( m_rect_details_on_btn.bottom - m_rect_details_on_btn.top ), TRUE );
	GetDlgItem( IDDETAILSON )->GetWindowRect( &m_rect_details_on_btn);
	ScreenToClient( &m_rect_details_on_btn);

	GetDlgItem( IDDETAILSOFF )->GetWindowRect( &m_rect_details_off_btn);
	ScreenToClient( &m_rect_details_off_btn);
	::MoveWindow( GetDlgItem( IDDETAILSOFF )->m_hWnd, first_btn_place.x, first_btn_place.y, 
		( m_rect_details_off_btn.right - m_rect_details_off_btn.left ),
		( m_rect_details_off_btn.bottom - m_rect_details_off_btn.top ), TRUE );
	GetDlgItem( IDDETAILSOFF )->GetWindowRect( &m_rect_details_off_btn);
	ScreenToClient( &m_rect_details_off_btn);
}

void CStMessageBox::PlaceOkDetailsBtn()
{
	POINT first_btn_place;
	CalculateButtonsStartingPoint( 2, IDOK, first_btn_place );

	GetDlgItem( IDOK )->GetWindowRect( &m_rect_ok_btn);
	ScreenToClient( &m_rect_ok_btn);
	::MoveWindow( GetDlgItem( IDOK )->m_hWnd, first_btn_place.x, first_btn_place.y, 
		( m_rect_ok_btn.right - m_rect_ok_btn.left ),
		( m_rect_ok_btn.bottom - m_rect_ok_btn.top ), TRUE );
	GetDlgItem( IDOK )->GetWindowRect( &m_rect_ok_btn);
	ScreenToClient( &m_rect_ok_btn);
	first_btn_place.x += m_rect_ok_btn.right - m_rect_ok_btn.left + SPACE_BETWEEN_BUTTONS;

	GetDlgItem( IDDETAILSON )->GetWindowRect( &m_rect_details_on_btn);
	ScreenToClient( &m_rect_details_on_btn);
	::MoveWindow( GetDlgItem( IDDETAILSON )->m_hWnd, first_btn_place.x, first_btn_place.y, 
		( m_rect_details_on_btn.right - m_rect_details_on_btn.left ),
		( m_rect_details_on_btn.bottom - m_rect_details_on_btn.top ), TRUE );
	GetDlgItem( IDDETAILSON )->GetWindowRect( &m_rect_details_on_btn);
	ScreenToClient( &m_rect_details_on_btn);

	GetDlgItem( IDDETAILSOFF )->GetWindowRect( &m_rect_details_off_btn);
	ScreenToClient( &m_rect_details_off_btn);
	::MoveWindow( GetDlgItem( IDDETAILSOFF )->m_hWnd, first_btn_place.x, first_btn_place.y, 
		( m_rect_details_off_btn.right - m_rect_details_off_btn.left ),
		( m_rect_details_off_btn.bottom - m_rect_details_off_btn.top ), TRUE );
	GetDlgItem( IDDETAILSOFF )->GetWindowRect( &m_rect_details_off_btn);
	ScreenToClient( &m_rect_details_off_btn);
}

void CStMessageBox::PlaceIcon()
{
	GetDlgItem( IDC_ICON_TYPE )->GetWindowRect( &m_rect_icon );
	ScreenToClient( &m_rect_icon );

	int width = m_rect_icon.right - m_rect_icon.left;
	int height = m_rect_icon.bottom - m_rect_icon.top;

	m_rect_icon.left = SPACE_BETWEEN_CONTROLS;
	m_rect_icon.top = SPACE_BETWEEN_CONTROLS;

	::MoveWindow( GetDlgItem( IDC_ICON_TYPE )->m_hWnd, m_rect_icon.left, m_rect_icon.top, width, height, TRUE );

	GetDlgItem( IDC_ICON_TYPE )->GetWindowRect( &m_rect_icon );
	ScreenToClient( &m_rect_icon );
}

void CStMessageBox::PlaceDlg(int _num_btns)
{
	GetWindowRect( &m_rect_dlg );
	
	RECT rDivider, rBtn;

	GetDlgItem(IDC_DIVIDER)->GetWindowRect( &rDivider );
	GetDlgItem(IDOK)->GetWindowRect( &rBtn );

	int width = ( ( ( rBtn.right - rBtn.left ) * _num_btns ) + ( SPACE_BETWEEN_CONTROLS * 2 ) +
		( SPACE_BETWEEN_BUTTONS * ( _num_btns - 1 ) ) );

	if( width < ( m_rect_text.right + SPACE_BETWEEN_CONTROLS ) )
	{
		width = m_rect_text.right + SPACE_BETWEEN_CONTROLS;
	}

	if( width < ( m_rect_detail_text.right + SPACE_BETWEEN_CONTROLS ) )
	{
		width = m_rect_detail_text.right + SPACE_BETWEEN_CONTROLS;
	}

	int height = rDivider.bottom - m_rect_dlg.top;

	::MoveWindow( m_hWnd, m_rect_dlg.left, m_rect_dlg.top, width, height, TRUE );

	GetWindowRect( &m_rect_dlg );
}

void CStMessageBox::HideYesNoBtn()
{
	GetDlgItem( IDYES )->EnableWindow( FALSE );
	GetDlgItem( IDYES )->ShowWindow( SW_HIDE );
	GetDlgItem( IDNO )->EnableWindow( FALSE );
	GetDlgItem( IDNO )->ShowWindow( SW_HIDE );
} 

void CStMessageBox::HideDetailsBtn()
{
	HideDetailsOnBtn();
	HideDetailsOffBtn();
}

void CStMessageBox::HideOkBtn()
{
	GetDlgItem( IDOK )->EnableWindow( FALSE );
	GetDlgItem( IDOK )->ShowWindow( SW_HIDE );
}

void CStMessageBox::HideDetailsOnBtn()
{
	GetDlgItem( IDDETAILSON )->EnableWindow( FALSE );
	GetDlgItem( IDDETAILSON )->ShowWindow( SW_HIDE );
}

void CStMessageBox::HideDetailsOffBtn()
{
	GetDlgItem( IDDETAILSOFF )->EnableWindow( FALSE );
	GetDlgItem( IDDETAILSOFF )->ShowWindow( SW_HIDE );
}


void CStMessageBox::ShowDetailsOnBtn()
{
	GetDlgItem( IDDETAILSON )->EnableWindow( TRUE );
	GetDlgItem( IDDETAILSON )->ShowWindow( SW_SHOW );
}

void CStMessageBox::ShowDetailsOffBtn()
{
	GetDlgItem( IDDETAILSOFF )->EnableWindow( TRUE );
	GetDlgItem( IDDETAILSOFF )->ShowWindow( SW_SHOW );
}