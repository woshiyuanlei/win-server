#include "StdAfx.h"
#include "Math.h"
//#include "TraceService.h"
#include "DataBase.h"

//////////////////////////////////////////////////////////////////////////

//�궨��
_COM_SMARTPTR_TYPEDEF(IADORecordBinding, __uuidof(IADORecordBinding));

//Ч������
#define EfficacyResult(hResult) { if (FAILED(hResult)) _com_issue_error(hResult); }

//////////////////////////////////////////////////////////////////////////

//���캯��
CADOError::CADOError()
{
	m_enErrorType = SQLException_None;
}

//��������
CADOError::~CADOError()
{
}

//�ӿڲ�ѯ
void * __cdecl CADOError::QueryInterface(const IID & Guid, DWORD dwQueryVer)
{
	//QUERYINTERFACE(IDataBaseException, Guid, dwQueryVer);
	//QUERYINTERFACE_IUNKNOWNEX(IDataBaseException, Guid, dwQueryVer);
	return NULL;
}

//���ô���
void CADOError::SetErrorInfo(enSQLException enErrorType, LPCTSTR pszDescribe)
{
	//���ô���
	m_enErrorType = enErrorType;
	m_strErrorDescribe = pszDescribe;

	//�׳�����
	//throw QUERY_ME_INTERFACE(IDataBaseException);

	return;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CDataBase::CDataBase() : m_dwResumeConnectCount(30L), m_dwResumeConnectTime(30L)
{
	//״̬����
	m_dwConnectCount = 0;
	m_dwConnectErrorTime = 0L;

	//��������
	m_DBCommand.CreateInstance(__uuidof(Command));
	m_DBRecordset.CreateInstance(__uuidof(Recordset));
	m_DBConnection.CreateInstance(__uuidof(Connection));

	//Ч������-
	assert/*ASSERT*/(m_DBCommand != NULL);
	assert/*ASSERT*/(m_DBRecordset != NULL);
	assert/*ASSERT*/(m_DBConnection != NULL);
	if (m_DBCommand == NULL) throw TEXT("���ݿ�������󴴽�ʧ��");
	if (m_DBRecordset == NULL) throw TEXT("���ݿ��¼�����󴴽�ʧ��");
	if (m_DBConnection == NULL) throw TEXT("���ݿ����Ӷ��󴴽�ʧ��");

	//���ñ���
	m_DBCommand->CommandType = adCmdStoredProc;

	return;
}

//��������
CDataBase::~CDataBase()
{
	//�ر�����
	CloseConnection();

	//�ͷŶ���
	m_DBCommand.Release();
	m_DBRecordset.Release();
	m_DBConnection.Release();

	return;
}

//�ӿڲ�ѯ
void * __cdecl CDataBase::QueryInterface(const IID & Guid, DWORD dwQueryVer)
{
	//QUERYINTERFACE(IDataBase, Guid, dwQueryVer);
	//QUERYINTERFACE_IUNKNOWNEX(IDataBase, Guid, dwQueryVer);
	return NULL;
}

//������-
VOID __cdecl CDataBase::OpenConnection()
{
	//�������ݿ�
	try
	{
		//�ر�����
		CloseConnection();

		//�������ݿ�
		EfficacyResult(m_DBConnection->Open(_bstr_t(/*m_strConnect*/m_strConnect.c_str()), L"", L"", adConnectUnspecified));  //whb
		m_DBConnection->CursorLocation = adUseClient;
		m_DBCommand->ActiveConnection = m_DBConnection;

		//���ñ���
		m_dwConnectCount = 0L;
		m_dwConnectErrorTime = 0L;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}
}

//�رռ�¼-
VOID __cdecl CDataBase::CloseRecordset()
{
	try
	{
		if (IsRecordsetOpened()) EfficacyResult(m_DBRecordset->Close());
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}
}

//�ر�����
VOID __cdecl CDataBase::CloseConnection()
{
	try
	{
		CloseRecordset();
		if ((m_DBConnection != NULL) && (m_DBConnection->GetState() != adStateClosed))
		{
			EfficacyResult(m_DBConnection->Close());
		}
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}
}

//��������
bool __cdecl CDataBase::TryConnectAgain(bool bFocusConnect, CComError * pComError)
{
	try
	{
		//�ж�����
		bool bReConnect = bFocusConnect;
		if (bReConnect == false)
		{
			DWORD dwNowTime = (DWORD)time(NULL);
			if ((m_dwConnectErrorTime + m_dwResumeConnectTime) > dwNowTime) bReConnect = true;
		}
		if ((bReConnect == false) && (m_dwConnectCount > m_dwResumeConnectCount)) bReConnect = true;

		//���ñ���
		m_dwConnectCount++;
		m_dwConnectErrorTime = (DWORD)time(NULL);
		if (bReConnect == false)
		{
			if (pComError != NULL) SetErrorInfo(SQLException_Connect, GetComErrorDescribe(*pComError));
			return false;
		}

		//��������-
		OpenConnection();
		return true;
	}
	catch (IDataBaseException * pIDataBaseException)
	{
		//�������Ӵ���-
		if (pComError != NULL) SetErrorInfo(SQLException_Connect, GetComErrorDescribe(*pComError));
		else throw pIDataBaseException;
	}

	return false;
}

//������Ϣ
bool __cdecl CDataBase::SetConnectionInfo(DWORD dwDBAddr, WORD wPort, LPCTSTR szDBName, LPCTSTR szUser, LPCTSTR szPassword)
{
	//Ч�����
	assert/*ASSERT*/(dwDBAddr != 0);
	assert/*ASSERT*/(szDBName != NULL);
	assert/*ASSERT*/(szUser != NULL);
	assert/*ASSERT*/(szPassword != NULL);

	BYTE a = (BYTE)((dwDBAddr >> 24) & 0xFF);
	BYTE b = (BYTE)((dwDBAddr >> 16) & 0xFF);
	BYTE c = (BYTE)((dwDBAddr >> 8) & 0xFF);
	BYTE d = (BYTE)(dwDBAddr & 0xFF);
	try
	{
		//���������ַ��� ȥMFC ����,CString �ĳ� string
		//m_strConnect.Format(TEXT("Provider=SQLOLEDB.1;Password=%s;Persist Security Info=True;User ID=%s;Initial Catalog=%d.%d.%d.%d;Data Source=%s,%ld;"),szPassword, szUser, szDBName, a, b, c, d, wPort);

		CHAR buf[1024] = { 0 };
		wsprintf(buf, TEXT("Provider=SQLOLEDB.1;Password=%s;Persist Security Info=True;User ID=%s;Initial Catalog=%d.%d.%d.%d;Data Source=%s,%ld;"), szPassword, szUser, szDBName, a, b, c, d, wPort);
		m_strConnect = buf;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}
	return true;
}

//�л���¼
VOID __cdecl CDataBase::NextRecordset()
{
	try
	{
		VARIANT lngRec;
		m_DBRecordset->NextRecordset(&lngRec);
		return;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

}

//������Ϣ
bool __cdecl CDataBase::SetConnectionInfo(LPCTSTR szDBAddr, WORD wPort, LPCTSTR szDBName, LPCTSTR szUser, LPCTSTR szPassword)
{
	//Ч�����
	assert/*ASSERT*/(szDBAddr != NULL);
	assert/*ASSERT*/(szDBName != NULL);
	assert/*ASSERT*/(szUser != NULL);
	assert/*ASSERT*/(szPassword != NULL);
	try
	{
		//���������ַ��� ȥMFC ����,CString �ĳ� string
		//m_strConnect.Format(TEXT("Provider=SQLOLEDB.1;Password=%s;Persist Security Info=True;User ID=%s;Initial Catalog=%s;Data Source=%s,%ld;"),
		//                    szPassword, szUser, szDBName, szDBAddr, wPort);

		CHAR buf[1024] = { 0 };
		wsprintf(buf,TEXT("Provider=SQLOLEDB.1;Password=%s;Persist Security Info=True;User ID=%s;Initial Catalog=%s;Data Source=%s,%ld;"),
			szPassword, szUser, szDBName, szDBAddr, wPort);
		m_strConnect = buf;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}
	return true;
}

//�Ƿ����Ӵ���
bool __cdecl CDataBase::IsConnectError()
{
	try
	{
		//״̬�ж�
		if (m_DBConnection == NULL) return true;
		if (m_DBConnection->GetState() == adStateClosed) return true;

		//�����ж�
		long lErrorCount = m_DBConnection->Errors->Count;
		if (lErrorCount > 0L)
		{
			ErrorPtr pError = NULL;
			for (long i = 0; i < lErrorCount; i++)
			{
				pError = m_DBConnection->Errors->GetItem(i);
				if (pError->Number == 0x80004005) return true;
			}
		}

		return false;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//�Ƿ��
bool __cdecl CDataBase::IsRecordsetOpened()
{
	if (m_DBRecordset == NULL) return false;
	if (m_DBRecordset->GetState() == adStateClosed) return false;
	return true;
}

//�����ƶ�
void __cdecl CDataBase::MoveToNext()
{
	try
	{
		m_DBRecordset->MoveNext();
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return;
}

//�Ƶ���ͷ
void __cdecl CDataBase::MoveToFirst()
{
	try
	{
		m_DBRecordset->MoveFirst();
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return;
}

//�Ƿ����
bool __cdecl CDataBase::IsRecordsetEnd()
{
	try
	{
		return (m_DBRecordset->EndOfFile == VARIANT_TRUE);
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return true;
}

//��ȡ��Ŀ
long __cdecl CDataBase::GetRecordCount()
{
	try
	{
		if (m_DBRecordset == NULL) return 0;
		return m_DBRecordset->GetRecordCount();
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return 0;
}

//��ȡ��С
long __cdecl CDataBase::GetActualSize(LPCTSTR pszParamName)
{
	assert/*ASSERT*/(pszParamName != NULL);
	try
	{
		return m_DBRecordset->Fields->Item[pszParamName]->ActualSize;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return -1;
}

//�󶨶���
VOID __cdecl CDataBase::BindToRecordset(CADORecordBinding * pBind)
{
	assert/*ASSERT*/(pBind != NULL);
	try
	{
		IADORecordBindingPtr pIBind(m_DBRecordset);
		pIBind->BindToRecordset(pBind);
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}
}

//��ȡ����
VOID __cdecl CDataBase::GetRecordsetValue(LPCTSTR pszItem, CDBVarValue & DBVarValue)
{
	assert/*ASSERT*/(pszItem != NULL);
	try
	{
		DBVarValue = m_DBRecordset->Fields->GetItem(pszItem)->Value;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}
}

//��ȡ����
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, BYTE & bValue)
{
	try
	{
		bValue = 0;
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
			case VT_BOOL:
			{
				bValue = (vtFld.boolVal != 0) ? 2 : 0;
				break;
			}
			case VT_I2:
			case VT_UI1:
			{
				bValue = (vtFld.iVal > 0) ? 2 : 0;
				break;
			}
			case VT_NULL:
			case VT_EMPTY:
			{
				bValue = 0;
				break;
			}
			default:
				bValue = (BYTE)vtFld.iVal;
		}
		return true;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ����
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, UINT & ulValue)
{
	try
	{
		ulValue = 0L;
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		if ((vtFld.vt != VT_NULL) && (vtFld.vt != VT_EMPTY)) ulValue = vtFld.lVal;
		return true;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ����
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, DOUBLE & dbValue)
{
	try
	{
		dbValue = 0.0L;
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
			case VT_R4:
			{
				dbValue = vtFld.fltVal;
				break;
			}
			case VT_R8:
			{
				dbValue = vtFld.dblVal;
				break;
			}
			case VT_DECIMAL:
			{
				dbValue = vtFld.decVal.Lo32;
				dbValue *= (vtFld.decVal.sign == 128) ? -1 : 1;
				dbValue /= pow((double)10, vtFld.decVal.scale);
				break;
			}
			case VT_UI1:
			{
				dbValue = vtFld.iVal;
				break;
			}
			case VT_I2:
			case VT_I4:
			{
				dbValue = vtFld.lVal;
				break;
			}
			case VT_NULL:
			case VT_EMPTY:
			{
				dbValue = 0.0L;
				break;
			}
			default:
				dbValue = vtFld.dblVal;
		}
		return true;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ����
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, LONG & lValue)
{
	try
	{
		lValue = 0L;
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		if ((vtFld.vt != VT_NULL) && (vtFld.vt != VT_EMPTY)) lValue = vtFld.lVal;
		return true;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ����
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, DWORD & ulValue)
{
	try
	{
		ulValue = 0L;
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		if ((vtFld.vt != VT_NULL) && (vtFld.vt != VT_EMPTY)) ulValue = vtFld.ulVal;
		return true;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ����
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, INT & nValue)
{
	try
	{
		nValue = 0;
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
			case VT_BOOL:
			{
				nValue = vtFld.boolVal;
				break;
			}
			case VT_I2:
			case VT_UI1:
			{
				nValue = vtFld.iVal;
				break;
			}
			case VT_NULL:
			case VT_EMPTY:
			{
				nValue = 0;
				break;
			}
			default:
				nValue = vtFld.iVal;
		}
		return true;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ����
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, __int64 & llValue)
{
	try
	{
		llValue = 0L;
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		if ((vtFld.vt != VT_NULL) && (vtFld.vt != VT_EMPTY)) llValue = vtFld.llVal;
		return true;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ����
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, TCHAR szBuffer[], UINT uSize)
{
	try
	{
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		if (vtFld.vt == VT_BSTR)
		{
			lstrcpy(szBuffer, (char*)_bstr_t(vtFld));
			return true;
		}
		return false;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ����
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, WORD & wValue)
{
	try
	{
		wValue = 0L;
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		if ((vtFld.vt != VT_NULL) && (vtFld.vt != VT_EMPTY)) wValue = (WORD)vtFld.ulVal;
		return true;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ���� ȥ��MFC���� COleDateTime �滻�� tagTime,��û���ԣ���֪������ǲ�����ȷ...whb
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, tagTime & Time/*COleDateTime & Time*/)
{
	try
	{
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
			case VT_DATE:
			{
				//COleDateTime TempTime(vtFld);
				//Time = TempTime;
				Time.time = vtFld.date;
				break;
			}
			case VT_EMPTY:
			case VT_NULL:
			{
				//Time.SetStatus(COleDateTime::null);
				break;
			}
			default:
				return false;
		}
		return true;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ����
bool __cdecl CDataBase::GetFieldValue(LPCTSTR lpFieldName, bool & bValue)
{
	try
	{
		_variant_t vtFld = m_DBRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
			case VT_BOOL:
			{
				bValue = (vtFld.boolVal == 0) ? false : true;
				break;
			}
			case VT_EMPTY:
			case VT_NULL:
			{
				bValue = false;
				break;
			}
			default:
				return false;
		}
		return true;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return false;
}

//��ȡ������ֵ
long __cdecl CDataBase::GetReturnValue()
{
	try
	{
		_ParameterPtr Parameter;
		long lParameterCount = m_DBCommand->Parameters->Count;
		for (long i = 0; i < lParameterCount; i++)
		{
			Parameter = m_DBCommand->Parameters->Item[i];
			if (Parameter->Direction == adParamReturnValue) return Parameter->Value.lVal;
		}
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return 0;
}

//ɾ������
void __cdecl CDataBase::ClearParameters()
{
	try
	{
		long lParameterCount = m_DBCommand->Parameters->Count;
		if (lParameterCount > 0L)
		{
			for (long i = lParameterCount; i > 0; i--)
			{
				m_DBCommand->Parameters->Delete(i - 1);
			}
		}
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return;
}


//��ò���
void __cdecl CDataBase::GetParameter(LPCTSTR pszParamName, CDBVarValue & DBVarValue)
{
	//Ч�����
	assert/*ASSERT*/(pszParamName != NULL);

	//��ȡ����
	try
	{
		DBVarValue.Clear();
		DBVarValue = m_DBCommand->Parameters->Item[pszParamName]->Value;
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}

	return;
}

//�������
void __cdecl CDataBase::AddParameter(LPCTSTR pszName, DataTypeEnum Type, ParameterDirectionEnum Direction, LONG lSize, CDBVarValue & DBVarValue)
{
	assert/*ASSERT*/(pszName != NULL);
	try
	{
		_ParameterPtr Parameter = m_DBCommand->CreateParameter(pszName, Type, Direction, lSize, DBVarValue);
		m_DBCommand->Parameters->Append(Parameter);
	}
	catch (CComError & ComError)
	{
		SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}
}

//ִ�����
VOID __cdecl CDataBase::ExecuteProcess(LPCTSTR pszSPName, bool bRecordset)
{
	assert/*ASSERT*/(pszSPName != NULL);
	try
	{
		//�رռ�¼��
		CloseRecordset();

		m_DBCommand->CommandText = pszSPName;

		//ִ������
		if (bRecordset == true)
		{
			m_DBRecordset->PutRefSource(m_DBCommand);
			m_DBRecordset->CursorLocation = adUseClient;
			EfficacyResult(m_DBRecordset->Open((IDispatch *)m_DBCommand, vtMissing, adOpenForwardOnly, adLockReadOnly, adOptionUnspecified));
		}
		else
		{
			m_DBConnection->CursorLocation = adUseClient;
			EfficacyResult(m_DBCommand->Execute(NULL, NULL, adExecuteNoRecords));
		}
	}
	catch (CComError & ComError)
	{
		if (IsConnectError() == true)	TryConnectAgain(false, &ComError);
		else SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}
}

//ִ������
VOID __cdecl CDataBase::ExecuteSentence(LPCTSTR pszCommand, bool bRecordset)
{
	assert/*ASSERT*/(pszCommand != NULL);
	try
	{
		m_DBConnection->CursorLocation = adUseClient;
		m_DBConnection->Execute(pszCommand, NULL, adExecuteNoRecords);
	}
	catch (CComError & ComError)
	{
		if (IsConnectError() == true)	TryConnectAgain(false, &ComError);
		else SetErrorInfo(SQLException_Syntax, GetComErrorDescribe(ComError));
	}
}

//��ȡ����
LPCTSTR CDataBase::GetComErrorDescribe(CComError & ComError)
{
	_bstr_t bstrDescribe(ComError.Description());
	//whb
	//m_strErrorDescribe.Format(TEXT("ADO ����0x%8x��%s"), ComError.Error(), (LPCTSTR)bstrDescribe);

	CHAR buf[1024] = { 0 };
	wsprintf(buf, TEXT("ADO ����0x%8x��%s"), ComError.Error(), (LPCTSTR)bstrDescribe);
	m_strConnect = buf;

	return /*m_strErrorDescribe*/m_strErrorDescribe.c_str();//whb
}

//���ô���
void CDataBase::SetErrorInfo(enSQLException enErrorType, LPCTSTR pszDescribe)
{
	m_ADOError.SetErrorInfo(enErrorType, pszDescribe);
	return;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CDataBaseEngine::CDataBaseEngine(void)
{
	//���ñ���
	m_bService = false;
	//m_pIDataBaseEngineSink = NULL;

	return;
}

//��������
CDataBaseEngine::~CDataBaseEngine(void)
{
}

//�ӿڲ�ѯ
void * __cdecl CDataBaseEngine::QueryInterface(const IID & Guid, DWORD dwQueryVer)
{
	//QUERYINTERFACE(IDataBaseEngine, Guid, dwQueryVer);
	//QUERYINTERFACE(IQueueServiceSink, Guid, dwQueryVer);
	//QUERYINTERFACE_IUNKNOWNEX(IDataBaseEngine, Guid, dwQueryVer);
	return NULL;
}

//ע��ӿ�
bool __cdecl CDataBaseEngine::SetDataBaseEngineSink(IServiceModule * pIDataBase)
{
	//Ч�����
	//assert/*ASSERT*/(pIDataBase != NULL);
	//assert/*ASSERT*/(m_pIDataBaseEngineSink == NULL);
	//if (pIDataBase == NULL) return false;
	//if (m_pIDataBaseEngineSink != NULL) return false;

	////��ѯ�ӿ�
	//m_pIDataBaseEngineSink = QUERY_OBJECT_PTR_INTERFACE(pIDataBase, IDataBaseEngineSink);
	//if (m_pIDataBaseEngineSink == NULL)
	//{
	//	CTraceService::TraceString(TEXT("���ݿ�������ҷ���ӿڻ�ȡʧ�ܣ��ҽӲ���ʧ��"), TraceLevel_Exception);
	//	return false;
	//}

	return true;
}

//��������
bool __cdecl CDataBaseEngine::StartService()
{
	////�ж�״̬
	//if (m_bService == true)
	//{
	//	CTraceService::TraceString(TEXT("���ݿ������ظ�������������������"), TraceLevel_Warning);
	//	return true;
	//}

	////��ҽӿ�
	//if (m_pIDataBaseEngineSink == NULL)
	//{
	//	CTraceService::TraceString(TEXT("���ݿ�������ҷ��񲻴���"), TraceLevel_Exception);
	//	return false;
	//}

	////���ö���
	//if (m_QueueServiceEvent.SetQueueServiceSink(QUERY_ME_INTERFACE(IDataBase)) == false)
	//{
	//	CTraceService::TraceString(TEXT("���ݿ���������з����ʧ��"), TraceLevel_Exception);
	//	return false;
	//}

	////�������
	//if (m_pIDataBaseEngineSink->OnDataBaseEngineStart(QUERY_ME_INTERFACE(IDataBase)) == false)
	//{
	//	CTraceService::TraceString(TEXT("���ݿ�������ҷ�������ʧ��"), TraceLevel_Exception);
	//	return false;
	//}

	//���ñ���
	m_bService = true;

	return true;
}

//ֹͣ����
bool __cdecl CDataBaseEngine::ConcludeService()
{
	//���ñ���
	m_bService = false;

	////ֹͣ�������
	////m_QueueServiceEvent.SetQueueServiceSink(NULL);

	////ֹͣ���
	//if (m_pIDataBaseEngineSink != NULL)
	//{
	//	m_pIDataBaseEngineSink->OnDataBaseEngineStop(QUERY_ME_INTERFACE(IDataBase));
	//	m_pIDataBaseEngineSink = NULL;
	//}

	//m_QueueServiceEvent.SetQueueServiceSink(NULL);

	return true;
}

//���нӿ�
void __cdecl CDataBaseEngine::OnQueueServiceSink(WORD wIdentifier, void * pBuffer, WORD wDataSize)
{
	//�ж�״̬
	if (m_bService == false) return;

	////������
	//switch (wIdentifier)
	//{
	//	case EVENT_DATABASE:
	//	{
	//		//Ч�����
	//		assert/*ASSERT*/(pBuffer != NULL);
	//		assert/*ASSERT*/(wDataSize >= sizeof(NTY_DataBaseEvent));
	//		if (wDataSize < sizeof(NTY_DataBaseEvent)) return;

	//		//��������
	//		NTY_DataBaseEvent * pDataBaseEvent = (NTY_DataBaseEvent *)pBuffer;
	//		WORD wHandleBuffer = wDataSize - sizeof(NTY_DataBaseEvent);

	//		//��������
	//		assert/*ASSERT*/(m_pIDataBaseEngineSink != NULL);
	//		m_pIDataBaseEngineSink->OnDataBaseEngineRequest(pDataBaseEvent->wRequestID, pDataBaseEvent->dwContextID, pDataBaseEvent + 1, wHandleBuffer);

	//		return;
	//	}
	//	case EVENT_TIMER://ʱ���¼�
	//	{
	//		//Ч�����
	//		assert/*ASSERT*/(pBuffer != NULL);
	//		assert/*ASSERT*/(wDataSize >= sizeof(NTY_TimerEvent));
	//		if (wDataSize < sizeof(NTY_TimerEvent)) return;

	//		//��������
	//		NTY_TimerEvent * pDataBaseEvent = (NTY_TimerEvent *)pBuffer;
	//		WORD wHandleBuffer = wDataSize - sizeof(NTY_TimerEvent);

	//		//��������
	//		assert/*ASSERT*/(m_pIDataBaseEngineSink != NULL);
	//		m_pIDataBaseEngineSink->OnDataBaseEngineTimer(pDataBaseEvent->dwTimerID, pDataBaseEvent->dwBindParameter);

	//		return;
	//	}
	//	case EVENT_CONTROL://�����¼�
	//	{
	//		//Ч�����
	//		assert/*ASSERT*/(pBuffer != NULL);
	//		assert/*ASSERT*/(wDataSize >= sizeof(NTY_ControlEvent));
	//		if (wDataSize < sizeof(NTY_ControlEvent)) return;

	//		//��������
	//		NTY_ControlEvent * pDataBaseEvent = (NTY_ControlEvent *)pBuffer;
	//		WORD wHandleBuffer = wDataSize - sizeof(NTY_ControlEvent);

	//		//��������
	//		assert/*ASSERT*/(m_pIDataBaseEngineSink != NULL);
	//		m_pIDataBaseEngineSink->OnDataBaseEngineControl(pDataBaseEvent->wControlID, pDataBaseEvent + 1, wHandleBuffer);

	//		return;
	//	}
	//	default:
	//	{
	//		assert/*ASSERT*/(false);
	//	}
	//}

	return;
}

//�����¼�
//////////////////////////////////////////////////////////////////////////
//ʱ���¼�
bool __cdecl CDataBaseEngine::PostDataBaseTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	//return m_QueueServiceEvent.PostTimerEvent(dwTimerID, dwBindParameter);
	return true;
}

//�����¼�
bool __cdecl CDataBaseEngine::PostDataBaseControl(WORD wControlID, VOID * pData, WORD wDataSize)
{
	//return m_QueueServiceEvent.PostControlEvent(wControlID, pData, wDataSize);
	return true;
}

//�����¼�
bool __cdecl CDataBaseEngine::PostDataBaseRequest(WORD wRequestID, DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//return m_QueueServiceEvent.PostDataBaseEvent(wRequestID, dwContextID, pData, wDataSize);
	return true;
}

//////////////////////////////////////////////////////////////////////////

//����������
extern "C" __declspec(dllexport) void * __cdecl CreateDataBaseEngine(/*const GUID & Guid, DWORD dwInterfaceVer*/)
{
	//��������
	IDataBaseEngine * pDataBaseEngine = NULL;
	try
	{
		pDataBaseEngine = new CDataBaseEngine();
		if (pDataBaseEngine == NULL) throw TEXT("����ʧ��");
		//void * pObject = pDataBaseEngine->QueryInterface(Guid, dwInterfaceVer);
		//if (pObject == NULL) throw TEXT("�ӿڲ�ѯʧ��");
		return /*pObject*/pDataBaseEngine;
	}
	catch (...) {}

	//��������
	if(pDataBaseEngine!=NULL){delete pDataBaseEngine;pDataBaseEngine=NULL;}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////

//����������
extern "C" __declspec(dllexport) void * __cdecl CreateDataBase(/*const GUID & Guid, DWORD dwInterfaceVer*/)
{
	//��������
	IDataBase * pDataBase = NULL;
	try
	{
		pDataBase = new CDataBase();
		if (pDataBase == NULL) throw TEXT("����ʧ��");
		//void * pObject = pDataBase->QueryInterface(Guid, dwInterfaceVer);
		//if (pObject == NULL) throw TEXT("�ӿڲ�ѯʧ��");
		return /*pObject*/pDataBase;
	}
	catch (...) {}

	//��������
	if(pDataBase!=NULL){delete pDataBase;pDataBase=NULL;}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////