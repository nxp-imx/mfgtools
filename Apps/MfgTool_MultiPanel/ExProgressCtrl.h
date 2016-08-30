/*
 * Copyright 2012-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

////////////////////////////////////////////////////////////////////////////////
// CExProgressCtrl class
class CExProgressCtrl : public CProgressCtrl
{
// Constructor / Destructor
public:
	CExProgressCtrl();
	virtual ~CExProgressCtrl();
	
// Operations
public:
    void SetBarColor(COLORREF crBarColor)
    {
        ::SendMessage(m_hWnd, PBM_SETBARCOLOR, 0, (LPARAM)crBarColor);
    }
    
    void SetBarBkColor(COLORREF crBarBkColor)
    {
        ::SendMessage(m_hWnd, PBM_SETBKCOLOR, 0, (LPARAM)crBarBkColor);
    }
    
    COLORREF GetBarColor() const
    {
       return( (COLORREF)::SendMessage(m_hWnd, PBM_GETBARCOLOR, 0, 0) );
    }
    
    COLORREF GetBarBkColor() const
    {
		return( (COLORREF)::SendMessage(m_hWnd, PBM_GETBKCOLOR, 0, 0) );
    }
    
protected:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    afx_msg LRESULT OnSetBarColor(WPARAM, LPARAM);
	afx_msg LRESULT OnSetBarBkColor(WPARAM, LPARAM);
	afx_msg LRESULT OnGetBarColor(WPARAM, LPARAM);
	afx_msg LRESULT OnGetBarBkColor(WPARAM, LPARAM);
	afx_msg LRESULT OnSetRange32(WPARAM nLow, LPARAM nHigh);
	afx_msg LRESULT OnSetPos(WPARAM nNewPos, LPARAM);
	
	DECLARE_MESSAGE_MAP()
	
	COLORREF  m_crBarColor;
	COLORREF  m_crBarBkColor;
	int       m_nPos;
	int       m_nLow;
    int       m_nHigh;
};
