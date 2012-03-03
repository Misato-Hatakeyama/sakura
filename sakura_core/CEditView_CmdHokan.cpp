/*!	@file
	@brief CEditView�N���X�̕⊮�֘A�R�}���h�����n�֐��Q

	@author genta
	@date	2005/01/10 �쐬
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, jepro
	Copyright (C) 2001, asa-o
	Copyright (C) 2003, Moca
	Copyright (C) 2004, Moca
	Copyright (C) 2005, genta

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/
#include "StdAfx.h"
#include "sakura_rc.h"
#include "CEditView.h"
#include "CEditDoc.h"
#include "Debug.h"
#include "etc_uty.h"
#include "charcode.h"  // 2006.06.28 rastiv
#include "CDocLineMgr.h"	// 2008.10.29 syat
#include "my_icmp.h"		// 2008.10.29 syat

/*!
	@brief �R�}���h��M�O�⊮����
	
	�⊮�E�B���h�E�̔�\��

	@date 2005.01.10 genta �֐���
*/
void CEditView::PreprocessCommand_hokan( int nCommand )
{
	/* �⊮�E�B���h�E���\������Ă���Ƃ��A���ʂȏꍇ�������ăE�B���h�E���\���ɂ��� */
	if( m_bHokan ){
		if( nCommand != F_HOKAN		//	�⊮�J�n�E�I���R�}���h
		 && nCommand != F_CHAR		//	��������
		 && nCommand != F_IME_CHAR	//	��������
		 ){
			m_pcEditDoc->m_cHokanMgr.Hide();
			m_bHokan = FALSE;
		}
	}
}

/*!
	�R�}���h���s��⊮����

	@author Moca
	@date 2005.01.10 genta �֐���
*/
void CEditView::PostprocessCommand_hokan(void)
{
	if( m_pShareData->m_Common.m_bUseHokan
 	 && FALSE == m_bExecutingKeyMacro	/* �L�[�{�[�h�}�N���̎��s�� */
	){
		CMemory	cmemData;

		/* �J�[�\�����O�̒P����擾 */
		if( 0 < GetLeftWord( &cmemData, 100 ) ){
			ShowHokanMgr( cmemData, FALSE );
		}else{
			if( m_bHokan ){
				m_pcEditDoc->m_cHokanMgr.Hide();
				m_bHokan = FALSE;
			}
		}
	}
}

/*!	�⊮�E�B���h�E��\������
	�E�B���h�E��\��������́AHokanMgr�ɔC����̂ŁAShowHokanMgr�̒m��Ƃ���ł͂Ȃ��B
	
	@param cmemData [in] �⊮���錳�̃e�L�X�g �uAb�v�Ȃǂ�����B
	@param bAutoDecided [in] ��₪1��������m�肷��

	@date 2005.01.10 genta CEditView_Command����ړ�
*/
void CEditView::ShowHokanMgr( CMemory& cmemData, BOOL bAutoDecided )
{
	/* �⊮�Ώۃ��[�h���X�g�𒲂ׂ� */
	CMemory		cmemHokanWord;
	int			nKouhoNum;
	POINT		poWin;
	/* �⊮�E�B���h�E�̕\���ʒu���Z�o */
	poWin.x = m_nViewAlignLeft
			 + (m_nCaretPosX - m_nViewLeftCol)
			  * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace );
	poWin.y = m_nViewAlignTop
			 + (m_nCaretPosY - m_nViewTopLine)
			  * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
	::ClientToScreen( m_hWnd, &poWin );
	poWin.x -= (
		cmemData.GetStringLength()
		 * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace )
	);

	/*	�⊮�E�B���h�E��\��
		�������AbAutoDecided == TRUE�̏ꍇ�́A�⊮��₪1�̂Ƃ��́A�E�B���h�E��\�����Ȃ��B
		�ڂ����́ASearch()�̐������Q�Ƃ̂��ƁB
	*/
	CMemory* pcmemHokanWord;
	if ( bAutoDecided ){
		pcmemHokanWord = &cmemHokanWord;
	}
	else {
		pcmemHokanWord = NULL;
	}
	nKouhoNum = m_pcEditDoc->m_cHokanMgr.Search(
		&poWin,
		m_nCharHeight,
		m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace,
		cmemData.GetStringPtr(),
		m_pcEditDoc->GetDocumentAttribute().m_szHokanFile,
		m_pcEditDoc->GetDocumentAttribute().m_bHokanLoHiCase,
		m_pcEditDoc->GetDocumentAttribute().m_bUseHokanByFile, // 2003.06.22 Moca
		pcmemHokanWord
	);
	/* �⊮���̐��ɂ���ē����ς��� */
	if (nKouhoNum <= 0) {				//	��△��
		if( m_bHokan ){
			m_pcEditDoc->m_cHokanMgr.Hide();
			m_bHokan = FALSE;
			// 2003.06.25 Moca ���s���Ă���A�r�[�v�����o���ĕ⊮�I���B
			ErrorBeep();
		}
	}
	else if( bAutoDecided && nKouhoNum == 1){ //	���1�̂݁��m��B
		if( m_bHokan ){
			m_pcEditDoc->m_cHokanMgr.Hide();
			m_bHokan = FALSE;
		}
		// 2004.05.14 Moca CHokanMgr::Search���ŉ��s���폜����悤�ɂ��A���ڏ���������̂���߂�
//		pszKouhoWord = cmemHokanWord.GetPtr( &nKouhoWordLen );
//		pszKouhoWord[nKouhoWordLen] = '\0';
		Command_WordDeleteToStart();
		Command_INSTEXT( TRUE, cmemHokanWord.GetStringPtr(), cmemHokanWord.GetStringLength(), TRUE );
	}
	else {
		m_bHokan = TRUE;
	}
	
	//	�⊮�I���B
	if ( !m_bHokan ){
		m_pShareData->m_Common.m_bUseHokan = FALSE;	//	���͕⊮�I���̒m�点
	}
}


/*!	���͕⊮
	Ctrl+Space�ł����ɓ����B
	CEditView::m_bHokan�F ���ݕ⊮�E�B���h�E���\������Ă��邩��\���t���O�B
	m_Common.m_bUseHokan�F���ݕ⊮�E�B���h�E���\������Ă���ׂ����ۂ�������킷�t���O�B

    @date 2001/06/19 asa-o �p�啶���������𓯈ꎋ����
                     ��₪1�̂Ƃ��͂���Ɋm�肷��
	@date 2001/06/14 asa-o �Q�ƃf�[�^�ύX
	                 �J���v���p�e�B�V�[�g���^�C�v�ʂɕύX
	@date 2000/09/15 JEPRO [Esc]�L�[��[x]�{�^���ł����~�ł���悤�ɕύX
	@date 2005/01/10 genta CEditView_Command����ړ�
*/
void CEditView::Command_HOKAN( void )
{
	if (m_pShareData->m_Common.m_bUseHokan == FALSE){
		m_pShareData->m_Common.m_bUseHokan = TRUE;
	}
retry:;
	/* �⊮���ꗗ�t�@�C�����ݒ肳��Ă��Ȃ��Ƃ��́A�ݒ肷��悤�ɑ����B */
	// 2003.06.22 Moca �t�@�C�������猟������ꍇ�ɂ͕⊮�t�@�C���̐ݒ�͕K�{�ł͂Ȃ�
	if( m_pcEditDoc->GetDocumentAttribute().m_bUseHokanByFile == FALSE &&
		0 == lstrlen( m_pcEditDoc->GetDocumentAttribute().m_szHokanFile 
	) ){
		ErrorBeep();
		if( IDYES == ::MYMESSAGEBOX( NULL, MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_TOPMOST, GSTR_APPNAME,
			"�⊮���ꗗ�t�@�C�����ݒ肳��Ă��܂���B\n�������ݒ肵�܂���?"
		) ){
			/* �^�C�v�ʐݒ� �v���p�e�B�V�[�g */
			if( !m_pcEditDoc->OpenPropertySheetTypes( 2, m_pcEditDoc->GetDocumentType() ) ){
				return;
			}
			goto retry;
		}
	}

	CMemory		cmemData;
	/* �J�[�\�����O�̒P����擾 */
	if( 0 < GetLeftWord( &cmemData, 100 ) ){
		ShowHokanMgr( cmemData, TRUE );
	}else{
		ErrorBeep();
		m_pShareData->m_Common.m_bUseHokan = FALSE;	//	���͕⊮�I���̂��m�点
	}
	return;
}


/*!
	�ҏW���f�[�^������͕⊮�L�[���[�h�̌���
	CHokanMgr����Ă΂��

	@return ��␔

	@author Moca
	@date 2003.06.25

	@date 2005/01/10 genta CEditView_Command����ړ�
*/
int CEditView::HokanSearchByFile(
		const char* pszKey,
		BOOL		bHokanLoHiCase,	//!< �p�啶���������𓯈ꎋ����
		CMemory**	ppcmemKouho,	//!< [IN/OUT] ���
		int			nKouhoNum,		//!< ppcmemKouho�̂��łɓ����Ă��鐔
		int			nMaxKouho		//!< Max��␔(0==������)
){
	const int nKeyLen = lstrlen( pszKey );
	int nLines = m_pcEditDoc->m_cDocLineMgr.GetLineCount();
	int i, j, nWordLen, nLineLen, nRet, nCharSize, nWordEnd;
	int nCurX, nCurY; // �����J�[�\���ʒu
	const char* pszLine;
	const char* word;
	nCurX = m_nCaretPosX_PHY;
	nCurY = m_nCaretPosY_PHY;
	bool bKeyStartWithMark;			//�L�[���L���Ŏn�܂邩
	bool bWordStartWithMark;		//��₪�L���Ŏn�܂邩

	// �L�[�̐擪���L��(#$@\)���ǂ�������
	bKeyStartWithMark = ( IS_KEYWORD_CHAR( pszKey[0] ) == 2 ? true : false );

	for( i = 0; i < nLines; i++ ){
		pszLine = m_pcEditDoc->m_cDocLineMgr.GetLineStrWithoutEOL( i, &nLineLen );

		for( j = 0; j < nLineLen; j += nCharSize ){
			nCharSize = CMemory::GetSizeOfChar( pszLine, nLineLen, j );

			// ���p�L���͌��Ɋ܂߂Ȃ�
			if ( (unsigned char)pszLine[j] < 0x80 && !IS_KEYWORD_CHAR( pszLine[j] ) )continue;

			// �L�[�̐擪���L���ȊO�̏ꍇ�A�L���Ŏn�܂�P��͌�₩��͂���
			if( !bKeyStartWithMark && IS_KEYWORD_CHAR( pszLine[j] ) == 2 )continue;

			// ���P��̊J�n�ʒu�����߂�
			word = pszLine + j;
			bWordStartWithMark = ( IS_KEYWORD_CHAR( pszLine[j] ) == 2 ? true : false );

			// ������ގ擾
			int kindPre = CDocLineMgr::WhatKindOfChar( pszLine, nLineLen, j );	// ������ގ擾

			// �S�p�L���͌��Ɋ܂߂Ȃ�
			if ( kindPre == CK_MBC_SPACE || kindPre == CK_MBC_NOVASU || kindPre == CK_MBC_DAKU ||
				 kindPre == CK_MBC_KIGO  || kindPre == CK_MBC_SKIGO )continue;

			// ���P��̏I���ʒu�����߂�
			nWordLen = nCharSize;
			nWordEnd = 0;
			for( j += nCharSize; j < nLineLen; j += nCharSize ){
				nWordEnd = j;			// ���[�v�𔲂������_�ŒP��̏I�����w��			
				nCharSize = CMemory::GetSizeOfChar( pszLine, nLineLen, j );

				// ���p�L���͊܂߂Ȃ�
				if ( (unsigned char)pszLine[j] < 0x80 && !IS_KEYWORD_CHAR( pszLine[j] ) )break;

				// ������ގ擾
				int kindCur = CDocLineMgr::WhatKindOfChar( pszLine, nLineLen, j );

				// �S�p�L���͌��Ɋ܂߂Ȃ��i�������R�S�T�U�W�X�Y�Z�[���_�͋��j
				if ( kindCur == CK_MBC_SPACE || kindCur == CK_MBC_KIGO || kindCur == CK_MBC_SKIGO ){
					break;
				}

				// ������ނ��ς������P��̐؂�ڂƂ���
				int kindMerge = CDocLineMgr::WhatKindOfTwoChars( kindPre, kindCur );
				if ( kindMerge == CK_NULL ) {	// kindPre��kindCur���ʎ�
					if( kindCur == CK_MBC_HIRA ) {
						kindMerge = kindCur;		// �Ђ炪�ȂȂ瑱�s
					}else if( bKeyStartWithMark && bWordStartWithMark && kindPre == CK_ETC ){
						kindMerge = kindCur;		// �L���Ŏn�܂�P��͐������ɂ߂�
					}else{
						j -= nCharSize;
						break;						// ����ȊO�͒P��̐؂��
					}
				}

				kindPre = kindMerge;
				nWordLen += nCharSize;				// ���̕�����
			}
			if( j >= nLineLen ) nWordEnd = nLineLen;

			if( nWordLen > 1020 ){ // CDicMgr���̐����ɂ�蒷������P��͖�������
				continue;
			}
			if( nKeyLen <= nWordLen ){
				if( bHokanLoHiCase ){
					nRet = my_memicmp( pszKey, word, nKeyLen );		// 2008.10.29 syat memicmp���}���`�o�C�g�����ɑ΂��s���Ȍ��ʂ����������ߏC��
				}else{
					nRet = memcmp( pszKey, word, nKeyLen );
				}
				if( 0 == nRet ){
					// �J�[�\���ʒu�̒P��͌�₩��͂���
					if( nCurY == i && nCurX <= nWordEnd && nWordEnd - nWordLen <= nCurX ){	// 2010.02.20 syat �C��// 2008.11.09 syat �C��
						continue;
					}
					if( NULL == *ppcmemKouho ){
						*ppcmemKouho = new CMemory;
						(*ppcmemKouho)->SetString( word, nWordLen );
						(*ppcmemKouho)->AppendString( "\n" );
						++nKouhoNum;
					}else{
						// �d�����Ă�����ǉ����Ȃ�
						int nLen;
						const char* ptr = (*ppcmemKouho)->GetStringPtr( &nLen );
						int nPosKouho;
						nRet = 1;
						// 2008.07.23 nasukoji	�啶���������𓯈ꎋ�̏ꍇ�ł����̐U�邢���Ƃ��͊��S��v�Ō���
						if( nWordLen < nLen ){
							if( '\n' == ptr[nWordLen] && 0 == memcmp( ptr, word, nWordLen )  ){
								nRet = 0;
							}else{
								int nPosKouhoMax = nLen - nWordLen - 1;
								for( nPosKouho = 1; nPosKouho < nPosKouhoMax; nPosKouho++ ){
									if( ptr[nPosKouho] == '\n' ){
										if( ptr[nPosKouho + nWordLen + 1] == '\n' ){
											if( 0 == memcmp( &ptr[nPosKouho + 1], word, nWordLen) ){
												nRet = 0;
												break;
											}else{
												nPosKouho += nWordLen;
											}
										}
									}
								}
							}
						}
						if( 0 == nRet ){
							continue;
						}
						(*ppcmemKouho)->AppendString( word, nWordLen );
						(*ppcmemKouho)->AppendString( "\n", 1 );
						++nKouhoNum;
					}
					if( 0 != nMaxKouho && nMaxKouho <= nKouhoNum ){
						return nKouhoNum;
					}
				}
			}
		}
	}
	return nKouhoNum;
}
/*[EOF]*/