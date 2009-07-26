/*!	@file
	@brief DIFF�����\��

	@author MIK
	@date	2002/05/25 ExecCmd ���Q�l��DIFF���s���ʂ���荞�ޏ����쐬
 	@date	2005/10/29	maru Diff�����\�������𕪗����A�_�C�A���O����ŁE�_�C�A���O�Ȃ��ł̗�������R�[��
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, GAE, YAZAKI, hor
	Copyright (C) 2002, hor, MIK
	Copyright (C) 2003, MIK, ryoji, genta
	Copyright (C) 2004, genta
	Copyright (C) 2005, maru
	Copyright (C) 2007, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "sakura_rc.h"
#include "etc_uty.h"
#include "global.h"
#include "CDlgDiff.h"
#include "CEditDoc.h"
#include "CEditView.h"
#include "CDocLine.h"
#include "CDocLineMgr.h"
#include "CWaitCursor.h"
#include "COsVersionInfo.h"
#include "mymessage.h"
#include "debug.h"

#define	SAKURA_DIFF_TEMP_PREFIX	"sakura_diff_"

/*!	�����\��
	@note	HandleCommand����̌Ăяo���Ή�(�_�C�A���O�Ȃ���)
	@author	maru
	@date	2005/10/28 ����܂ł�Command_Diff��ViewDiffInfo�ɖ��̕ύX
*/
void CEditView::Command_Diff( const char *szTmpFile2, int nFlgOpt )
{
	bool	bTmpFile1 = false;
	char	szTmpFile1[_MAX_PATH * 2];

	if( -1 == ::GetFileAttributes( szTmpFile2 ) )
	{
		::MYMESSAGEBOX( m_hWnd,	MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
			_T( "�����R�}���h���s�͎��s���܂����B\n\n��r����t�@�C����������܂���B" ) );
		return;
	}

	//���t�@�C��
	if (m_pcEditDoc->IsModified() == FALSE ) strcpy( szTmpFile1, m_pcEditDoc->GetFilePath());
	else if (MakeDiffTmpFile ( szTmpFile1, NULL )) bTmpFile1 = true;
	else return;

	//�����\��
	ViewDiffInfo(szTmpFile1, szTmpFile2, nFlgOpt);

	//�ꎞ�t�@�C�����폜����
	if( bTmpFile1 ) unlink( szTmpFile1 );

	return;

}

/*!	�����\��
	@note	HandleCommand����̌Ăяo���Ή�(�_�C�A���O�����)
	@author	MIK
	@date	2002/05/25
	@date	2002/11/09 �ҏW���t�@�C��������
	@date	2005/10/29 maru �ꎞ�t�@�C���쐬������MakeDiffTmpFile�ֈړ�
*/
void CEditView::Command_Diff_Dialog( void )
{
	CDlgDiff	cDlgDiff;
	bool	bTmpFile1 = false, bTmpFile2 = false;
	char	szTmpFile1[_MAX_PATH * 2];
	char	szTmpFile2[_MAX_PATH * 2];

	//DIFF�����\���_�C�A���O��\������
	if( FALSE == cDlgDiff.DoModal( m_hInstance, m_hWnd, (LPARAM)m_pcEditDoc,
		m_pcEditDoc->GetFilePath(),
		m_pcEditDoc->IsModified() ) )
	{
		return;
	}
	
	//���t�@�C��
	if (m_pcEditDoc->IsModified() == FALSE ) strcpy( szTmpFile1, m_pcEditDoc->GetFilePath());
	else if (MakeDiffTmpFile ( szTmpFile1, NULL )) bTmpFile1 = true;
	else return;
		
	//����t�@�C��
	if (cDlgDiff.m_bIsModifiedDst == FALSE ) strcpy( szTmpFile2, cDlgDiff.m_szFile2);
	else if (MakeDiffTmpFile ( szTmpFile2, cDlgDiff.m_hWnd_Dst )) bTmpFile2 = true;
	else 
	{
		if( bTmpFile1 ) unlink( szTmpFile1 );
		return;
	}
	
	//�����\��
	ViewDiffInfo(szTmpFile1, szTmpFile2, cDlgDiff.m_nDiffFlgOpt);
	
	
	//�ꎞ�t�@�C�����폜����
	if( bTmpFile1 ) unlink( szTmpFile1 );
	if( bTmpFile2 ) unlink( szTmpFile2 );

	return;
}

/*!	�����\��
	@param	pszFile1	[in]	���t�@�C����
	@param	pszFile2	[in]	����t�@�C����
    @param  nFlgOpt     [in]    0b000000000
                                    ||||||+--- -i ignore-case         �啶�����������ꎋ
                                    |||||+---- -w ignore-all-space    �󔒖���
                                    ||||+----- -b ignore-space-change �󔒕ύX����
                                    |||+------ -B ignore-blank-lines  ��s����
                                    ||+------- -t expand-tabs         TAB-SPACE�ϊ�
                                    |+--------    (�ҏW���̃t�@�C�������t�@�C��)
                                    +---------    (DIFF�������Ȃ��Ƃ��Ƀ��b�Z�[�W�\��)
	@note	HandleCommand����̌Ăяo���Ή�(�_�C�A���O�Ȃ���)
	@author	MIK
	@date	2002/05/25
	@date	2005/10/28	��Command_Diff����֐����̕ύX�B
						Command_Diff_Dialog�����łȂ��VCommand_Diff
						������Ă΂��֐��Bmaru
*/
void CEditView::ViewDiffInfo( 
	const char	*pszFile1,
	const char	*pszFile2,
	int			nFlgOpt )
/*
	bool	bFlgCase,		//�啶�����������ꎋ
	bool	bFlgBlank,		//�󔒖���
	bool	bFlgWhite,		//�󔒕ύX����
	bool	bFlgBLine,		//��s����
	bool	bFlgTabSpc,		//TAB-SPACE�ϊ�
	bool	bFlgFile12,		//�ҏW���̃t�@�C�������t�@�C��
*/
{
	char	cmdline[1024];
	HANDLE	hStdOutWrite, hStdOutRead;
//	CDlgCancel	cDlgCancel;
	CWaitCursor	cWaitCursor( m_hWnd );
	int		nFlgFile12 = 1;

	/* exe�̂���t�H���_ */
	char	szExeFolder[_MAX_PATH + 1];

	GetExedir( cmdline, _T("diff.exe") );
	SplitPath_FolderAndFile( cmdline, szExeFolder, NULL );

	//	From Here Dec. 28, 2002 MIK
	//	diff.exe�̑��݃`�F�b�N
	if( -1 == ::GetFileAttributes( cmdline ) )
	{
		::MYMESSAGEBOX( m_hWnd,	MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
			_T( "�����R�}���h���s�͎��s���܂����B\n\nDIFF.EXE ��������܂���B" ) );
		return;
	}

	//������DIFF��������������B
	if( m_pcEditDoc->m_cDocLineMgr.IsDiffUse() )
		Command_Diff_Reset();
		//m_pcEditDoc->m_cDocLineMgr.ResetAllDiffMark();

	PROCESS_INFORMATION	pi;
	ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );

	//�q�v���Z�X�̕W���o�͂Ɛڑ�����p�C�v���쐬
	SECURITY_ATTRIBUTES	sa;
	ZeroMemory( &sa, sizeof(SECURITY_ATTRIBUTES) );
	sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle       = TRUE;
	sa.lpSecurityDescriptor = NULL;
	hStdOutRead = hStdOutWrite = 0;
	if( CreatePipe( &hStdOutRead, &hStdOutWrite, &sa, 1000 ) == FALSE )
	{
		//�G���[�B�΍�����
		return;
	}

	//�p���s�\�ɂ���
	DuplicateHandle( GetCurrentProcess(), hStdOutRead,
				GetCurrentProcess(), NULL,
				0, FALSE, DUPLICATE_SAME_ACCESS );

	//CreateProcess�ɓn��STARTUPINFO���쐬
	STARTUPINFO	sui;
	ZeroMemory( &sui, sizeof(STARTUPINFO) );
	sui.cb          = sizeof(STARTUPINFO);
	sui.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	sui.wShowWindow = SW_HIDE;
	sui.hStdInput   = GetStdHandle( STD_INPUT_HANDLE );
	sui.hStdOutput  = hStdOutWrite;
	sui.hStdError   = hStdOutWrite;

	//�I�v�V�������쐬����
	char	szOption[16];	// "-cwbBt"
	strcpy( szOption, "-" );
	if( nFlgOpt & 0x0001 ) strcat( szOption, "i" );	//-i ignore-case         �啶�����������ꎋ
	if( nFlgOpt & 0x0002 ) strcat( szOption, "w" );	//-w ignore-all-space    �󔒖���
	if( nFlgOpt & 0x0004 ) strcat( szOption, "b" );	//-b ignore-space-change �󔒕ύX����
	if( nFlgOpt & 0x0008 ) strcat( szOption, "B" );	//-B ignore-blank-lines  ��s����
	if( nFlgOpt & 0x0010 ) strcat( szOption, "t" );	//-t expand-tabs         TAB-SPACE�ϊ�
	if( strcmp( szOption, "-" ) == 0 ) strcpy( szOption, "" );	//�I�v�V�����Ȃ�
	if( nFlgOpt & 0x0020 ) nFlgFile12 = 0;
	else                   nFlgFile12 = 1;

	//	To Here Dec. 28, 2002 MIK

	//OS�o�[�W�����擾
	{
		COsVersionInfo cOsVer;
		//�R�}���h���C��������쐬(MAX:1024)
		if (cOsVer.IsWin32NT()){
			wsprintf( cmdline, "cmd.exe /C \"\"%s\\%s\" %s \"%s\" \"%s\"\"",
					szExeFolder,	//sakura.exe�p�X
					"diff.exe",		//diff.exe
					szOption,		//diff�I�v�V����
					( nFlgFile12 ? pszFile2 : pszFile1 ),
					( nFlgFile12 ? pszFile1 : pszFile2 )
				);
		}
		else{
			wsprintf( cmdline, "command.com /C \"%s\\%s\" %s \"%s\" \"%s\"",
					szExeFolder,	//sakura.exe�p�X
					"diff.exe",		//diff.exe
					szOption,		//diff�I�v�V����
					( nFlgFile12 ? pszFile2 : pszFile1 ),
					( nFlgFile12 ? pszFile1 : pszFile2 )
				);
		}
	}

	//�R�}���h���C�����s
	if( CreateProcess( NULL, cmdline, NULL, NULL, TRUE,
			CREATE_NEW_CONSOLE, NULL, NULL, &sui, &pi ) == FALSE )
	{
			::MYMESSAGEBOX( NULL, MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
				"�����R�}���h���s�͎��s���܂����B\n\n%s", cmdline );
		goto finish;
	}

	{
		DWORD	read_cnt;
		DWORD	new_cnt;
		char	work[1024];
		int		j;
		bool	bLoopFlag = true;
		bool	bLineHead = true;	//�s����
		bool	bDiffInfo = false;	//DIFF���
		int		nDiffLen = 0;		//DIFF���
		char	szDiffData[100];	//DIFF���
		bool	bFirst = true;	//�擪���H	//@@@ 2003.05.31 MIK

		//���f�_�C�A���O�\��
//		cDlgCancel.DoModeless( m_hInstance, m_hwndParent, IDD_EXECRUNNING );

		//���s���ʂ̎�荞��
		do {
			//�������̃��[�U�[������\�ɂ���
//			if( !::BlockingHook( cDlgCancel.m_hWnd ) )
//			{
//				bDiffInfo = false;
//				break;
//			}

			//���f�{�^�������`�F�b�N
//			if( cDlgCancel.IsCanceled() )
//			{
				//�w�肳�ꂽ�v���Z�X�ƁA���̃v���Z�X�������ׂẴX���b�h���I�������܂��B
//				::TerminateProcess( pi.hProcess, 0 );
//				bDiffInfo = false;
//				break;
//			}

			//�v���Z�X���I�����Ă��Ȃ����m�F
			// Jul. 04, 2003 genta CPU��100%�g���ʂ����̂�h������ 200msec�x��
			// Jan. 23, 2004 genta
			// �q�v���Z�X�̏o�͂��ǂ�ǂ�󂯎��Ȃ��Ǝq�v���Z�X��
			// ��~���Ă��܂����߁C�҂����Ԃ�200ms����20ms�Ɍ��炷
			if( WaitForSingleObject( pi.hProcess, 20 ) == WAIT_OBJECT_0 )
			{
				//�I�����Ă���΃��[�v�t���O��FALSE�Ƃ���
				//���������[�v�̏I�������� �v���Z�X�I�� && �p�C�v����
				bLoopFlag = FALSE;
			}

			new_cnt = 0;
			if( PeekNamedPipe( hStdOutRead, NULL, 0, NULL, &new_cnt, NULL ) )
			{
				while( new_cnt > 0 )												//�ҋ@���̂��̂�����
				{
					if( new_cnt >= sizeof(work) - 2 )							//�p�C�v����ǂݏo���ʂ𒲐�
					{
						new_cnt = sizeof(work) - 2;
					}
					ReadFile( hStdOutRead, &work[0], new_cnt, &read_cnt, NULL );	//�p�C�v����ǂݏo��
					if( read_cnt == 0 )
					{
						// Jan. 23, 2004 genta while�ǉ��̂��ߐ����ύX
						break;
					}

					//@@@ 2003.05.31 MIK
					//	�擪��Binary files�Ȃ�o�C�i���t�@�C���̂��߈Ӗ��̂��鍷��������Ȃ�����
					if( bFirst )
					{
						bFirst = false;
						if( strncmp( work, "Binary files ", strlen( "Binary files " ) ) == 0 )
						{
							::MYMESSAGEBOX( NULL, MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
								"DIFF�������s�����Ƃ����t�@�C���̓o�C�i���t�@�C���ł��B" );
							goto finish;
						}
					}

					//�ǂݏo������������`�F�b�N����
					for( j = 0; j < (int)read_cnt/*-1*/; j++ )
					{
						if( bLineHead )
						{
							if( work[j] != '\n' && work[j] != '\r' )
							{
								bLineHead = false;
							
								//DIFF���̎n�܂肩�H
								if( work[j] >= '0' && work[j] <= '9' )
								{
									bDiffInfo = true;
									nDiffLen = 0;
									szDiffData[nDiffLen++] = work[j];
								}
								/*
								else if( work[j] == '<' || work[j] == '>' || work[j] == '-' )
								{
									bDiffInfo = false;
									nDiffLen = 0;
								}
								*/
							}
						}
						else
						{
							//�s���ɒB�������H
							if( work[j] == '\n' || work[j] == '\r' )
							{
								//DIFF��񂪂���Ή�͂���
								if( bDiffInfo == true && nDiffLen > 0 )
								{
									szDiffData[nDiffLen] = '\0';
									AnalyzeDiffInfo( szDiffData, nFlgFile12 );
									nDiffLen = 0;
								}
								
								bDiffInfo = false;
								bLineHead = true;
							}
							else if( bDiffInfo == true )
							{
								//DIFF���ɒǉ�����
								szDiffData[nDiffLen++] = work[j];
								if( nDiffLen >= 99 )
								{
									nDiffLen = 0;
									bDiffInfo = false;
								}
							}
						}
					}
					// Jan. 23, 2004 genta
					// �q�v���Z�X�̏o�͂��ǂ�ǂ�󂯎��Ȃ��Ǝq�v���Z�X��
					// ��~���Ă��܂����߁C�o�b�t�@����ɂȂ�܂łǂ�ǂ�ǂݏo���D
					new_cnt = 0;
					if( ! PeekNamedPipe( hStdOutRead, NULL, 0, NULL, &new_cnt, NULL ) ){
						break;
					}
					Sleep(0); // Jan. 23, 2004 genta �^�X�N�X�C�b�`�𑣂�
				}
			}
		} while( bLoopFlag || new_cnt > 0 );

		//�c����DIFF��񂪂���Ή�͂���
		if( bDiffInfo == true && nDiffLen > 0 )
		{
			szDiffData[nDiffLen] = '\0';
			AnalyzeDiffInfo( szDiffData, nFlgFile12 );
		}
	}


	//DIFF������������Ȃ������Ƃ��Ƀ��b�Z�[�W�\��
	if( nFlgOpt & 0x0040 )
	{
		if( false == m_pcEditDoc->m_cDocLineMgr.IsDiffUse() )
		{
			::MYMESSAGEBOX( m_hWnd,	MB_OK | MB_ICONINFORMATION, GSTR_APPNAME,
				"DIFF�����͌�����܂���ł����B" );
		}
	}


finish:
	//�I������
	CloseHandle( hStdOutWrite );
	CloseHandle( hStdOutRead  );
	if( pi.hProcess ) CloseHandle( pi.hProcess );
	if( pi.hThread  ) CloseHandle( pi.hThread  );

	//���������r���[���X�V
	for( int v = 0; v < 4; ++v )
		if( m_pcEditDoc->m_nActivePaneIndex != v )
			m_pcEditDoc->m_cEditViewArr[v].Redraw();
	Redraw();

	return;
}

/*!	DIFF����������͂��}�[�N�o�^
	@param	pszDiffInfo	[in]	�V�t�@�C����
	@param	nFlgFile12	[in]	�ҏW���t�@�C����...
									0	�t�@�C��1(���t�@�C��)
									1	�t�@�C��2(�V�t�@�C��)
	@author	MIK
	@date	2002/05/25
*/
void CEditView::AnalyzeDiffInfo( 
	const char	*pszDiffInfo,
	int		nFlgFile12 )
{
	/*
	 * 99a99		���t�@�C��99�s�̎��s�ɐV�t�@�C��99�s���ǉ����ꂽ�B
	 * 99a99,99		���t�@�C��99�s�̎��s�ɐV�t�@�C��99�`99�s���ǉ����ꂽ�B
	 * 99c99		���t�@�C��99�s���V�t�@�C��99�s�ɕύX���ꂽ�B
	 * 99,99c99,99	���t�@�C��99�`99�s���V�t�@�C��99�`99�s�ɕύX���ꂽ�B
	 * 99d99		���t�@�C��99�s���V�t�@�C��99�s�̎��s����폜���ꂽ�B
	 * 99,99d99		���t�@�C��99�`99�s���V�t�@�C��99�s�̎��s����폜���ꂽ�B
	 * s1,e1 mode s2,e2
	 * �擪�̏ꍇ0�̎��s�ƂȂ邱�Ƃ�����
	 */
	const char	*q;
	int		s1, e1, s2, e2;
	char	mode;

	//�O���t�@�C���̊J�n�s
	s1 = 0;
	for( q = pszDiffInfo; *q; q++ )
	{
		if( *q == ',' ) break;
		if( *q == 'a' || *q == 'c' || *q == 'd' ) break;
		//�s�ԍ��𒊏o
		if( *q >= '0' && *q <= '9' ) s1 = s1 * 10 + (*q - '0');
		else return;
	}
	if( ! *q ) return;

	//�O���t�@�C���̏I���s
	if( *q != ',' )
	{
		//�J�n�E�I���s�ԍ��͓���
		e1 = s1;
	}
	else
	{
		e1 = 0;
		for( q++; *q; q++ )
		{
			if( *q == 'a' || *q == 'c' || *q == 'd' ) break;
			//�s�ԍ��𒊏o
			if( *q >= '0' && *q <= '9' ) e1 = e1 * 10 + (*q - '0');
			else return;
		}
	}
	if( ! *q ) return;

	//DIFF���[�h���擾
	mode = *q;

	//�㔼�t�@�C���̊J�n�s
	s2 = 0;
	for( q++; *q; q++ )
	{
		if( *q == ',' ) break;
		//�s�ԍ��𒊏o
		if( *q >= '0' && *q <= '9' ) s2 = s2 * 10 + (*q - '0');
		else return;
	}

	//�㔼�t�@�C���̏I���s
	if( *q != ',' )
	{
		//�J�n�E�I���s�ԍ��͓���
		e2 = s2;
	}
	else
	{
		e2 = 0;
		for( q++; *q; q++ )
		{
			//�s�ԍ��𒊏o
			if( *q >= '0' && *q <= '9' ) e2 = e2 * 10 + (*q - '0');
			else return;
		}
	}

	//�s���ɒB���ĂȂ���΃G���[
	if( *q ) return;

	//���o����DIFF��񂩂�s�ԍ��ɍ����}�[�N��t����
	if( 0 == nFlgFile12 )	//�ҏW���t�@�C���͋��t�@�C��
	{
		if     ( mode == 'a' ) m_pcEditDoc->m_cDocLineMgr.SetDiffMarkRange( MARK_DIFF_DELETE, s1    , e1     );
		else if( mode == 'c' ) m_pcEditDoc->m_cDocLineMgr.SetDiffMarkRange( MARK_DIFF_CHANGE, s1 - 1, e1 - 1 );
		else if( mode == 'd' ) m_pcEditDoc->m_cDocLineMgr.SetDiffMarkRange( MARK_DIFF_APPEND, s1 - 1, e1 - 1 );
	}
	else	//�ҏW���t�@�C���͐V�t�@�C��
	{
		if     ( mode == 'a' ) m_pcEditDoc->m_cDocLineMgr.SetDiffMarkRange( MARK_DIFF_APPEND, s2 - 1, e2 - 1 );
		else if( mode == 'c' ) m_pcEditDoc->m_cDocLineMgr.SetDiffMarkRange( MARK_DIFF_CHANGE, s2 - 1, e2 - 1 );
		else if( mode == 'd' ) m_pcEditDoc->m_cDocLineMgr.SetDiffMarkRange( MARK_DIFF_DELETE, s2    , e2     );
	}

	return;
}

/*!	���̍�����T���C����������ړ�����
*/
void CEditView::Command_Diff_Next( void )
{
	int			nX = 0;
	int			nY;
	int			nYOld;
	BOOL		bFound = FALSE;
	BOOL		bRedo = TRUE;

	nY = m_nCaretPosY_PHY;
	nYOld = nY;

re_do:;	
	if( m_pcEditDoc->m_cDocLineMgr.SearchDiffMark( nY, 1 /* ������� */, &nY ) )
	{
		bFound = TRUE;
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( nX, nY, &nX, &nY );
		if( m_bSelectingLock )
		{
			if( !IsTextSelected() ) BeginSelectArea();
		}
		else
		{
			if( IsTextSelected() ) DisableSelectArea( TRUE );
		}
		MoveCursor( nX, nY, TRUE );
		if( m_bSelectingLock )
		{
			ChangeSelectAreaByCurrentCursor( nX, nY );
		}
	}

	if( m_pShareData->m_Common.m_bSearchAll )
	{
		if( !bFound		// ������Ȃ�����
		 && bRedo )		// �ŏ��̌���
		{
			nY = 0 - 1;	/* 1��O���w�� */
			bRedo = FALSE;
			goto re_do;		// �擪����Č���
		}
	}
	if( bFound )
	{
		if( nYOld >= nY ) SendStatusMessage( "���擪����Č������܂���" );
	}
	else
	{
		SendStatusMessage( "��������܂���ł���" );
		if( m_pShareData->m_Common.m_bNOTIFYNOTFOUND )	/* ������Ȃ��Ƃ����b�Z�[�W��\�� */
			::MYMESSAGEBOX( m_hWnd,	MB_OK | MB_ICONINFORMATION, GSTR_APPNAME,
				"���(��) �ɍ�����������܂���B" );
	}

	return;
}



/*!	�O�̍�����T���C����������ړ�����
*/
void CEditView::Command_Diff_Prev( void )
{
	int			nX = 0;
	int			nY;
	int			nYOld;
	BOOL		bFound = FALSE;
	BOOL		bRedo = TRUE;

	nY = m_nCaretPosY_PHY;
	nYOld = nY;

re_do:;
	if( m_pcEditDoc->m_cDocLineMgr.SearchDiffMark( nY, 0 /* �O������ */, &nY ) )
	{
		bFound = TRUE;
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( nX, nY, &nX, &nY );
		if( m_bSelectingLock )
		{
			if( !IsTextSelected() ) BeginSelectArea();
		}
		else
		{
			if( IsTextSelected() ) DisableSelectArea( TRUE );
		}
		MoveCursor( nX, nY, TRUE );
		if( m_bSelectingLock )
		{
			ChangeSelectAreaByCurrentCursor( nX, nY );
		}
	}

	if( m_pShareData->m_Common.m_bSearchAll )
	{
		if( !bFound	// ������Ȃ�����
		 && bRedo )	// �ŏ��̌���
		{
			nY = m_pcEditDoc->m_cLayoutMgr.GetLineCount() - 1 + 1;	/* 1��O���w�� */
			bRedo = FALSE;
			goto re_do;	// ��������Č���
		}
	}
	if( bFound )
	{
		if( nYOld <= nY ) SendStatusMessage( "����������Č������܂���" );
	}
	else
	{
		SendStatusMessage( "��������܂���ł���" );
		if( m_pShareData->m_Common.m_bNOTIFYNOTFOUND )	/* ������Ȃ��Ƃ����b�Z�[�W��\�� */
			::MYMESSAGEBOX( m_hWnd,	MB_OK | MB_ICONINFORMATION, GSTR_APPNAME,
				"�O��(��) �ɍ�����������܂���B" );
	}

	return;
}

/*!	�����\���̑S����
	@author	MIK
	@date	2002/05/26
*/
void CEditView::Command_Diff_Reset( void )
{
	m_pcEditDoc->m_cDocLineMgr.ResetAllDiffMark();

	//���������r���[���X�V
	for( int v = 0; v < 4; ++v )
		if( m_pcEditDoc->m_nActivePaneIndex != v )
			m_pcEditDoc->m_cEditViewArr[v].Redraw();
	Redraw();
	return;
}

/*!	�ꎞ�t�@�C�����쐬����
	@author	MIK
	@date	2002/05/26
	@date	2005/10/29	�����ύXconst char* �� char*
						�ꎞ�t�@�C�����̎擾�����������ł����Ȃ��Bmaru
*/
BOOL CEditView::MakeDiffTmpFile( char* filename, HWND hWnd )
{
	const char*	pLineData;
	int		nLineLen;
	int		y;
	FILE	*fp;

	char	*pszTmpName;
	
	pszTmpName = _tempnam( NULL, SAKURA_DIFF_TEMP_PREFIX );
	if( NULL == pszTmpName )
	{
		::MYMESSAGEBOX( NULL, MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
			"�����R�}���h���s�͎��s���܂����B" );
		return FALSE;
	}

	strcpy( filename, pszTmpName );
	free( pszTmpName );

	//�������H
	if( NULL == hWnd )
	{
		CEOL	cEol( m_pcEditDoc->m_cSaveLineCode );
		FILETIME	filetime;
		return (BOOL)m_pcEditDoc->m_cDocLineMgr.WriteFile( 
			filename, 
			m_pcEditDoc->m_hWnd, 
			NULL,
			m_pcEditDoc->m_nCharCode,
			&filetime,
			cEol,
			m_pcEditDoc->m_bBomExist);	//	Jul. 26, 2003 ryoji BOM
	}

	fp = fopen( filename, "wb" );
	if( NULL == fp )
	{
		::MYMESSAGEBOX( NULL, MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
			"�����R�}���h���s�͎��s���܂����B\n\n�ꎞ�t�@�C�����쐬�ł��܂���B" );
		return FALSE;
	}

	y = 0;

	// �s(���s�P��)�f�[�^�̗v��
	if( hWnd )
	{
		pLineData = m_pShareData->m_szWork;
		nLineLen = ::SendMessage( hWnd, MYWM_GETLINEDATA, y, 0 );
	}
	else
	{
		pLineData = m_pcEditDoc->m_cDocLineMgr.GetLineStr( y, &nLineLen );
	}

	while( 1 )
	{
		if( 0 == nLineLen || NULL == pLineData ) break;

		if( hWnd && nLineLen > sizeof( m_pShareData->m_szWork ) )
		{
			// �ꎞ�o�b�t�@�𒴂��Ă���
			fclose( fp );
			::MYMESSAGEBOX( NULL, MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
				"�����R�}���h���s�͎��s���܂����B\n\n�s���������܂��B" );
			unlink( filename );		//�֐��̎��s�Ɏ��s�����Ƃ��A�ꎞ�t�@�C���̍폜�͊֐����ōs���B2005.10.29
			return FALSE;
		}

		if( 1 != fwrite( pLineData, nLineLen, 1, fp ) )
		{
			fclose( fp );
			::MYMESSAGEBOX( NULL, MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
				"�����R�}���h���s�͎��s���܂����B\n\n�ꎞ�t�@�C�����쐬�ł��܂���B" );
			unlink( filename );		//�֐��̎��s�Ɏ��s�����Ƃ��A�ꎞ�t�@�C���̍폜�͊֐����ōs���B2005.10.29
			return FALSE;
		}

		y++;

		// �s(���s�P��)�f�[�^�̗v�� 
		if( hWnd ) nLineLen = ::SendMessage( hWnd, MYWM_GETLINEDATA, y, 0 );
		else       pLineData = m_pcEditDoc->m_cDocLineMgr.GetLineStr( y, &nLineLen );
	}

	fclose( fp );

	return TRUE;
}


/*[EOF]*/