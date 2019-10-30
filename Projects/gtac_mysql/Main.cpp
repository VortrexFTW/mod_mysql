
#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include <GalacticInterfaces.h>
#include <GalacticStrongPtr.h>

#include <mysql.h>
#include <malloc.h>

// The modules internal name (Also used for the namespace name)
MODULE_MAIN("mysql");

SDK::Class g_ConnectionClass;
SDK::Class g_ResultClass;

class CConnection
{
public:
	inline CConnection(MYSQL* pMySQL) : m_pMySQL(pMySQL) {}
	inline ~CConnection()
	{
		Close();
	}

	MYSQL* m_pMySQL;

	void Close();
};

void CConnection::Close()
{
	if (m_pMySQL != nullptr)
	{
		mysql_close(m_pMySQL);
		m_pMySQL = nullptr;
	}
}

class CMySQLResult
{
public:
	inline CMySQLResult(MYSQL_RES* pResult) : m_pResult(pResult) {}
	inline ~CMySQLResult()
	{
		Free();
	}

	MYSQL_RES* m_pResult;

	void Free();
};

void CMySQLResult::Free()
{
	if (m_pResult != nullptr)
	{
		mysql_free_result(m_pResult);
		m_pResult = nullptr;
	}
}

void ModuleRegister()
{
	//printf("Registering module");
	g_ConnectionClass = SDK::Class("Connection");
	g_ResultClass = SDK::Class("Result");
	//printf("Classes created");

	SDK::RegisterFunction("connect", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		const char* szHostname = State.CheckString(0);
		const char* szUsername = State.CheckString(1);
		const char* szPassword = State.CheckString(2);
		const char* szDatabase = State.CheckString(3);

		int iPort;
		State.CheckNumber(4, iPort);

		MYSQL *pMySQL;
		if (!(pMySQL = mysql_init(NULL))) {
			return pState->SetError("[MySQL] connect: MySQL initialization failed");
		}

		if (!mysql_real_connect(pMySQL, szHostname, szUsername, szPassword, szDatabase, iPort, 0, 0))
		{
			mysql_close(pMySQL);
			return pState->SetError("[MySQL] connect: %s (%i)", mysql_error(pMySQL), mysql_errno(pMySQL));
		}

		SDK::ClassValue<CConnection, g_ConnectionClass> Value(new CConnection(pMySQL));
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.connect function registered");

	g_ConnectionClass.RegisterFunction("query", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		const char* szQuery = State.CheckString(0);

		if (mysql_query(pMySQL, szQuery))
		{
			return pState->SetError("%s (%i)", mysql_error(pMySQL), mysql_errno(pMySQL));
		}

		MYSQL_RES* pResult = mysql_store_result(pMySQL);
		if (!pResult)
		{
			SDK::NullValue Value;
			State.Return(Value);
			return true;
		}

		SDK::ClassValue<CMySQLResult, g_ResultClass> Value(new CMySQLResult(pResult));
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.query function registered");

	g_ConnectionClass.RegisterFunction("close", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		pThis->Close();

		return true;

		SDK_ENDTRY;
	});

	//printf("Connection.close function registered");

	g_ConnectionClass.AddProperty("ping", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		SDK::BooleanValue Value(mysql_ping(pMySQL) == 0);
		State.Return(Value);

		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.ping property registered");

	g_ConnectionClass.RegisterFunction("escapeString", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		size_t Length;
		const char* szString = State.CheckString(0, &Length);

		char* pszBuffer = (char*)alloca(Length * 2 + 1);

		auto Length2 = mysql_real_escape_string(pMySQL, pszBuffer, szString, Length);
		if (Length2 == ~0)
			return pState->SetError("Failed to escape the string");

		SDK::StringValue Value(pszBuffer, Length2);
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.escapeString function registered");

	g_ConnectionClass.RegisterFunction("selectDatabase", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		const char* pszString = State.CheckString(0);

		if (mysql_select_db(pMySQL, pszString))
			return pState->SetError("%s (%i)", mysql_error(pMySQL), mysql_errno(pMySQL));

		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.selectDatabase function registered");

	g_ConnectionClass.RegisterFunction("changeUser", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		const char* szUser = State.CheckString(0);
		const char* szPassword = State.CheckString(1);
		const char* szDatabase = State.CheckString(2);

		if (mysql_change_user(pMySQL, szUser, szPassword, szDatabase))
		{
			return pState->SetError("%s (%i)", mysql_error(pMySQL), mysql_errno(pMySQL));
		}

		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.changeUser function registered");

	g_ConnectionClass.AddProperty("insertId", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		SDK::NumberValue Value((uint64_t)mysql_insert_id(pMySQL));
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.insertId property registered");

	g_ConnectionClass.AddProperty("affectedRows", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		SDK::NumberValue Value((uint64_t)mysql_affected_rows(pMySQL));
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.affectedRows property registered");

	g_ConnectionClass.AddProperty("warningCount", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		SDK::NumberValue Value((uint32_t)mysql_warning_count(pMySQL));
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.warningCount property registered");

	g_ConnectionClass.RegisterFunction("info", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		const char* szInfo = mysql_info(pMySQL);

		if (!szInfo)
		{
			SDK::NullValue Value;
			State.Return(Value);
			return true;
		}

		SDK::StringValue Value(szInfo);
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.info property registered");

	g_ConnectionClass.AddProperty("errorNum", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		SDK::NumberValue Value((uint32_t)mysql_errno(pMySQL));
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.errorNum property registered");

	g_ConnectionClass.AddProperty("error", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CConnection, g_ConnectionClass>();

		MYSQL* pMySQL = pThis->m_pMySQL;
		if (pMySQL == nullptr)
			return pState->SetError("MySQL connection is closed");

		SDK::StringValue Value(mysql_error(pMySQL));
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Connection.error property registered");

	g_ResultClass.RegisterFunction("free", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CMySQLResult, g_ResultClass>();

		MYSQL_RES* pResult = pThis->m_pResult;
		if (pResult == nullptr)
			return pState->SetError("MySQL result is deleted");

		pThis->Free();
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Result.free function registered");

	g_ResultClass.AddProperty("numRows", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CMySQLResult, g_ResultClass>();

		MYSQL_RES* pResult = pThis->m_pResult;
		if (pResult == nullptr)
			return pState->SetError("MySQL result is deleted");

		SDK::NumberValue Value((uint64_t)mysql_num_rows(pResult));
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Result.numRows property registered");

	g_ResultClass.RegisterFunction("fetchAssoc", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CMySQLResult, g_ResultClass>();

		MYSQL_RES* pResult = pThis->m_pResult;
		if (pResult == nullptr)
			return pState->SetError("MySQL result is deleted");

		SDK::NumberValue Value((uint32_t)mysql_num_fields(pResult));
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Result.fetchAssoc property registered");

	g_ResultClass.RegisterFunction("fetchAssoc", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CMySQLResult, g_ResultClass>();

		MYSQL_RES* pResult = pThis->m_pResult;
		if (pResult == nullptr)
			return pState->SetError("MySQL result is deleted");

		MYSQL_ROW pRow = mysql_fetch_row(pResult);
		if (!pRow)
		{
			SDK::NullValue Value;
			State.Return(Value);
			return true;
		}

		uint32_t uiColumns = mysql_num_fields(pResult);
		if (uiColumns > 0)
		{
			SDK::DictionaryValue AssocDictionary;

			mysql_field_seek(pResult, 0);
			for (uint32_t ui = 0; ui < uiColumns; ui++)
			{
				MYSQL_FIELD* pField = mysql_fetch_field(pResult);

				if (!pField || !pRow[ui])
				{
					SDK::NullValue Value;
					AssocDictionary.Set(pField->name, Value);
				}
				else
				{
					switch (pField->type)
					{
						case MYSQL_TYPE_TINY:
						case MYSQL_TYPE_SHORT:
						case MYSQL_TYPE_LONG:
						case MYSQL_TYPE_LONGLONG:
						case MYSQL_TYPE_INT24:
						case MYSQL_TYPE_YEAR:
						case MYSQL_TYPE_BIT:
							{
								SDK::NumberValue Value(atoi(pRow[ui]));
								AssocDictionary.Set(pField->name, Value);
								break;
							}

						case MYSQL_TYPE_DECIMAL:
						case MYSQL_TYPE_NEWDECIMAL:
						case MYSQL_TYPE_FLOAT:
						case MYSQL_TYPE_DOUBLE:
							{
								SDK::NumberValue Value(atof(pRow[ui]));
								AssocDictionary.Set(pField->name, Value);
								break;
							}

						case MYSQL_TYPE_NULL:
							{
								SDK::NullValue Value;
								AssocDictionary.Set(pField->name, Value);
								break;
							}

						default:
							{
								SDK::StringValue Value(pRow[ui]);
								AssocDictionary.Set(pField->name, Value);
								break;
							}
					}
				}
			}

			State.Return(AssocDictionary);
			return true;
		}

		SDK::NullValue Value;
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Result.fetchAssoc function registered");

	g_ResultClass.RegisterFunction("fetchRow", [](Galactic3D::Interfaces::INativeState* pState, int32_t argc, void* pUser) {
		SDK_TRY;

		SDK::State State(pState);

		auto pThis = State.CheckThis<CMySQLResult, g_ResultClass>();

		MYSQL_RES* pResult = pThis->m_pResult;
		if (pResult == nullptr)
			return pState->SetError("MySQL result is deleted");
	
		MYSQL_ROW pRow = mysql_fetch_row(pResult);
		if (!pRow)
		{
			SDK::NullValue Value;
			State.Return(Value);
			return true;
		}

		uint32_t uiColumns = mysql_num_fields(pResult);
		if (uiColumns > 0)
		{
			SDK::ArrayValue Array;

			mysql_field_seek(pResult, 0);
			for (uint32_t ui = 0; ui < uiColumns; ui++)
			{
				MYSQL_FIELD* pField = mysql_fetch_field(pResult);

				if (!pField || !pRow[ui])
				{
					SDK::NullValue Value;
					Array.Insert(Value);
				}
				else
				{
					switch (pField->type)
					{
						case MYSQL_TYPE_TINY:
						case MYSQL_TYPE_SHORT:
						case MYSQL_TYPE_LONG:
						case MYSQL_TYPE_LONGLONG:
						case MYSQL_TYPE_INT24:
						case MYSQL_TYPE_YEAR:
						case MYSQL_TYPE_BIT:
							{
								SDK::NumberValue Value(atoi(pRow[ui]));
								Array.Insert(Value);
								break;
							}

						case MYSQL_TYPE_DECIMAL:
						case MYSQL_TYPE_NEWDECIMAL:
						case MYSQL_TYPE_FLOAT:
						case MYSQL_TYPE_DOUBLE:
							{
								SDK::NumberValue Value(atof(pRow[ui]));
								Array.Insert(Value);
								break;
							}

						case MYSQL_TYPE_NULL:
							{
								SDK::NullValue Value;
								Array.Insert(Value);
								break;
							}

						default:
							{
								SDK::StringValue Value(pRow[ui]);
								Array.Insert(Value);
								break;
							}
					}
				}
			}

			State.Return(Array);
			return true;
		}

		SDK::NullValue Value;
		State.Return(Value);
		return true;

		SDK_ENDTRY;
	});
	
	//printf("Result.fetchRow function registered");
}

void ModuleUnregister()
{
}
