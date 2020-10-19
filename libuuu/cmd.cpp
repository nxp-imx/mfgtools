/*
* Copyright 2018 NXP.
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
* Neither the name of the NXP Semiconductor nor the names of its
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

#include <regex>
#include <iterator>
#include <memory>
#include <string.h>
#include "cmd.h"
#include "libcomm.h"
#include "liberror.h"
#include "libuuu.h"
#include "config.h"
#include "trans.h"
#include "sdps.h"
#include <atomic>
#include "buffer.h"
#include "sdp.h"
#include "fastboot.h"
#include <sys/stat.h>
#include <thread>

#include <stdio.h>  
#include <stdlib.h>  

static CmdMap g_cmd_map;
static CmdObjCreateMap g_cmd_create_map;
static string g_cmd_list_file;

int get_string_in_square_brackets(const std::string &cmd, std::string &context);
int parser_cmd_list_file(shared_ptr<FileBuffer> pbuff, CmdMap *pCmdMap = nullptr);
std::string remove_square_brackets(const std::string &cmd);

template <class T>
void * create_object() { return new T; }

typedef void * (*FN)();

FN g_fn = create_object<int>;

CmdCtx::~CmdCtx()
{
}

CmdBase::~CmdBase()
{
}

int CmdBase::parser(char *p)
{
	if (p != nullptr)
		m_cmd = p;

	size_t pos = 0;
	string param = get_next_param(m_cmd, pos);

	if (param.find(':') != string::npos)
		param = get_next_param(m_cmd, pos);

	int index = 0;

	while (pos < m_cmd.size())
	{
		param = get_next_param(m_cmd, pos);

		struct Param *pp = nullptr;

		if (m_NoKeyParam)
		{
			if (index > m_param.size())
			{
				set_last_err_string("More parameter then expected");
				return -1;
			}
			pp = &(m_param[index]);
			index++;
		}
		else
		{
			for (size_t i = 0; i < m_param.size(); i++)
			{
				string key = string(m_param[i].key);
				if (compare_str(param, key, m_param[i].ignore_case))
				{
					pp = &(m_param[i]);
					break;
				}
			}
		}

		if (pp == nullptr)
		{
			string err;
			err = "unknown Option";
			err += param;
			set_last_err_string(err);
			return -1;
		}

		if (pp->type == Param::Type::e_uint32)
		{
			if (!m_NoKeyParam)
				param = get_next_param(m_cmd, pos);
			*(uint32_t*)pp->pData = str_to_uint32(param);
		}

		if (pp->type == Param::Type::e_string_filename)
		{
			if (!m_NoKeyParam)
				param = get_next_param(m_cmd, pos);
			*(string*)pp->pData = param;

			if (!check_file_exist(param))
				return -1;
		}

		if (pp->type == Param::Type::e_string)
		{
			if (!m_NoKeyParam)
				param = get_next_param(m_cmd, pos);
			*(string*)pp->pData = param;
		}

		if (pp->type == Param::Type::e_bool)
		{
			*(bool*)pp->pData = true;
		}

		if (pp->type == Param::Type::e_null)
		{
		}
	}

	if (m_bCheckTotalParam)
	{
		if (index < m_param.size())
		{
			string str;
			str += "Missed: ";
			str += m_param[index].Error;
			set_last_err_string(str);
			return -1;
		}
	}
	return 0;
}

int CmdBase::parser_protocal(char *p, size_t &pos)
{
	if (p)
		m_cmd = *p;

	string prot = get_next_param(m_cmd, pos, ':');
	string param;
	if (get_string_in_square_brackets(prot, param))
		return -1;

	if (!param.empty())
	{
		size_t param_pos = 0;
		string s = get_next_param(param, param_pos);

		if (s == "-t")
		{
			string timeout;
			timeout = get_next_param(param, param_pos);
			m_timeout = str_to_uint32(timeout);
		}
		else
		{
			string err;
			err = "Unknown option: ";
			err += s;
			err += " for protocol: ";
			err += remove_square_brackets(prot);
			set_last_err_string(err);
			return -1;
		}
	}
	return 0;
}

int CmdBase::dump()
{
	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_CMD_INFO;

	string str =  m_cmd;
	str += "\n";
	nt.str = (char*)str.c_str();
	call_notify(nt);

	return 0;
}

int CmdList::run_all(CmdCtx *p, bool dry)
{
	CmdList::iterator it;
	int ret;

	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_CMD_TOTAL;
	nt.total = size();
	call_notify(nt);

	int i = 0;

	for (it = begin(); it != end(); it++, i++)
	{
		uuu_notify nt;

		nt.type = uuu_notify::NOTIFY_CMD_INDEX;
		nt.index = i;
		call_notify(nt);

		nt.type = uuu_notify::NOTIFY_CMD_START;
		nt.str = (char *)(*it)->get_cmd().c_str();
		call_notify(nt);

		if (dry)
			ret = (*it)->dump();
		else
			ret = (*it)->run(p);

		nt.type = uuu_notify::NOTIFY_CMD_END;
		nt.status = ret;
		call_notify(nt);
		if (ret)
			return ret;

		if ((*it)->get_lastcmd())
				break;
	}
	return ret;
}

int CmdMap::run_all(const std::string &protocol, CmdCtx *p, bool dry_run)
{
	if (find(protocol) == end())
	{
		set_last_err_id(-1);
		std::string err;
		err.append("Unknown Protocal:");
		err.append(protocol);
		set_last_err_string(err);
		return -1;
	}
	return at(protocol)->run_all(p, dry_run);
}

string get_next_param(const string &cmd, size_t &pos, char sperate)
{
	string str;
	if (pos == string::npos)
		return str;
	if (pos >= cmd.size())
		return str;

	//trim left space
	while (cmd[pos] == sperate && pos < cmd.size())
		pos++;

	bool quate = false;
	size_t end = string::npos;

	for (size_t s = pos; s < cmd.size(); s++)
	{
		if (cmd[s] == '"')
			quate = !quate;

		if (!quate && cmd[s] == sperate)
		{
			end = s;
			break;
		}
	}

	if (end == cmd.npos)
		end = cmd.size();

	str = cmd.substr(pos, end - pos);
	pos = end + 1;

	return str;
}

string remove_square_brackets(const string &cmd)
{
	size_t sz=cmd.find('[');
	return cmd.substr(0, sz);
}

int get_string_in_square_brackets(const string &cmd, string &context)
{
	size_t start = cmd.find('[');
	if (start == string::npos)
	{
		context.clear();
		return 0;
	}

	size_t end = cmd.find(']', start);
	if (end == string::npos)
	{
		set_last_err_string("missed ]");
		return -1;
	}

	context = cmd.substr(start + 1, end - start - 1);
	return 0;
}

template<typename T, uint64_t MAX_VAL>
T str_to_uint(const std::string &str, bool * conversion_succeeded)
{
	if (conversion_succeeded) *conversion_succeeded = false;

	int base = 10;
	if (str.size() > 2)
	{
		if (str.substr(0, 2).compare("0x") == 0)
		{
			base = 16;
		}
	}

	try {
		const auto tmp_val = std::stoull(str, nullptr, base);
		if (tmp_val <= MAX_VAL)
		{
			if (conversion_succeeded) *conversion_succeeded = true;
			return static_cast<T>(tmp_val);
		}
	}  catch (const std::invalid_argument &) {
	} catch (const std::out_of_range &) {
	}

	set_last_err_string("Conversion of string to unsigned failed");

	return MAX_VAL;
}

uint16_t str_to_uint16(const string &str, bool * conversion_suceeded)
{
	return str_to_uint<uint16_t, UINT16_MAX>(str, conversion_suceeded);
}

uint32_t str_to_uint32(const string &str, bool * conversion_suceeded)
{
	return str_to_uint<uint32_t, UINT32_MAX>(str, conversion_suceeded);
}

uint64_t str_to_uint64(const string &str, bool * conversion_suceeded)
{
	return str_to_uint<uint64_t, UINT64_MAX>(str, conversion_suceeded);
}

template <class T> shared_ptr<CmdBase> new_cmd_obj(char *p)
{
	return shared_ptr<CmdBase>(new T(p));
}

CmdObjCreateMap::CmdObjCreateMap()
{
	(*this)["CFG:"] = new_cmd_obj<CfgCmd>;

	(*this)["SDPS:BOOT"] = new_cmd_obj<SDPSCmd>;

	(*this)["SDP:DCD"] = new_cmd_obj<SDPDcdCmd>;
	(*this)["SDP:JUMP"] = new_cmd_obj<SDPJumpCmd>;
	(*this)["SDP:RDMEM"] = new_cmd_obj<SDPReadMemCmd>;
	(*this)["SDP:WRMEM"] = new_cmd_obj<SDPWriteMemCmd>;
	(*this)["SDP:WRITE"] = new_cmd_obj<SDPWriteCmd>;
	(*this)["SDP:STATUS"] = new_cmd_obj<SDPStatusCmd>;
	(*this)["SDP:BOOT"] = new_cmd_obj<SDPBootCmd>;
	(*this)["SDP:BLOG"] = new_cmd_obj<SDPBootlogCmd>;

	(*this)["SDPU:JUMP"] = new_cmd_obj<SDPJumpCmd>;
	(*this)["SDPU:WRITE"] = new_cmd_obj<SDPWriteCmd>;
	(*this)["SDPU:BLOG"] = new_cmd_obj<SDPBootlogCmd>;

	(*this)["SDPV:JUMP"] = new_cmd_obj<SDPJumpCmd>;
	(*this)["SDPV:WRITE"] = new_cmd_obj<SDPWriteCmd>;
	(*this)["SDPV:BLOG"] = new_cmd_obj<SDPBootlogCmd>;

	(*this)["FB:GETVAR"] = new_cmd_obj<FBGetVar>;
	(*this)["FASTBOOT:GETVAR"] = new_cmd_obj<FBGetVar>;
	(*this)["FB:UCMD"] = new_cmd_obj<FBUCmd>;
	(*this)["FASTBOOT:UCMD"] = new_cmd_obj<FBUCmd>;
	(*this)["FB:ACMD"] = new_cmd_obj<FBACmd>;
	(*this)["FASTBOOT:ACMD"] = new_cmd_obj<FBACmd>;
	(*this)["FB:DOWNLOAD"] = new_cmd_obj<FBDownload>;
	(*this)["FASTBOOT:DOWNLOAD"] = new_cmd_obj<FBDownload>;
	(*this)["FB:FLASH"] = new_cmd_obj<FBFlashCmd>;
	(*this)["FASTBOOT:FLASH"] = new_cmd_obj<FBFlashCmd>;
	(*this)["FB:ERASE"] = new_cmd_obj<FBEraseCmd>;
	(*this)["FASTBOOT:ERASE"] = new_cmd_obj<FBEraseCmd>;
	(*this)["FB:REBOOT"] = new_cmd_obj<FBRebootCmd>;
	(*this)["FASTBOOT:REBOOT"] = new_cmd_obj<FBRebootCmd>;
	(*this)["FB:OEM"] = new_cmd_obj<FBOemCmd>;
	(*this)["FASTBOOT:OEM"] = new_cmd_obj<FBOemCmd>;
	(*this)["FB:FLASHING"] = new_cmd_obj<FBFlashingCmd>;
	(*this)["FASTBOOT:FLASHING"] = new_cmd_obj<FBFlashingCmd>;
	(*this)["FB:SET_ACTIVE"] = new_cmd_obj<FBSetActiveCmd>;
	(*this)["FASTBOOT:SET_ACTIVE"] = new_cmd_obj<FBSetActiveCmd>;
	(*this)["FB:CONTINUE"] = new_cmd_obj<FBContinueCmd>;
	(*this)["FASTBOOT:CONTINUE"] = new_cmd_obj<FBContinueCmd>;

	(*this)["FB:UPDATE-SUPER"] = new_cmd_obj<FBUpdateSuper>;
	(*this)["FASTBOOT:UPDATE-SUPER"] = new_cmd_obj<FBUpdateSuper>;
	(*this)["FB:CREATE-LOGICAL-PARTITION"] = new_cmd_obj<FBCreatePartition>;
	(*this)["FASTBOOT:CREATE-LOGICAL-PARTITION"] = new_cmd_obj<FBCreatePartition>;
	(*this)["FB:DELETE-LOGICAL-PARTITION"] = new_cmd_obj<FBDelPartition>;
	(*this)["FASTBOOT:DELETE-LOGICAL-PARTITION"] = new_cmd_obj<FBDelPartition>;
	(*this)["FB:RESIZE-LOGICAL-PARTITION"] = new_cmd_obj<FBResizePartition>;
	(*this)["FASTBOOT:RESIZE-LOGICAL-PARTITION"] = new_cmd_obj<FBResizePartition>;

	(*this)["FBK:UCMD"] = new_cmd_obj<FBUCmd>;
	(*this)["FBK:ACMD"] = new_cmd_obj<FBACmd>;
	(*this)["FBK:SYNC"] = new_cmd_obj<FBSyncCmd>;
	(*this)["FBK:UCP"] = new_cmd_obj<FBCopy>;

	(*this)["_ALL:DONE"] = new_cmd_obj<CmdDone>;
	(*this)["_ALL:DELAY"] = new_cmd_obj<CmdDelay>;
	(*this)["_ALL:SH"] = new_cmd_obj<CmdShell>;
	(*this)["_ALL:SHELL"] = new_cmd_obj<CmdShell>;
	(*this)["_ALL:<"] = new_cmd_obj<CmdShell>;
	(*this)["_ALL:@"] = new_cmd_obj<CmdEnv>;

}

shared_ptr<CmdBase> create_cmd_obj(string cmd)
{
	string param;
	size_t pos = 0;
	param = get_next_param(cmd, pos, ':');
	param = remove_square_brackets(param);
	param += ":";
	param = str_to_upper(param);

	if (g_cmd_create_map.find(param) == g_cmd_create_map.end())
	{
		string s = param;
		param = get_next_param(cmd, pos);
		s += str_to_upper(param);
		if (g_cmd_create_map.find(s) != g_cmd_create_map.end())
			return g_cmd_create_map[s]((char*)cmd.c_str());

		string commoncmd = "_ALL:";
		commoncmd += str_to_upper(param);
		if (g_cmd_create_map.find(commoncmd) != g_cmd_create_map.end())
			return g_cmd_create_map[commoncmd]((char*)cmd.c_str());
	}
	else
	{
		return g_cmd_create_map[param]((char*)cmd.c_str());
	}

	string err;
	err = "Unknown Command:";
	err += cmd;
	set_last_err_string(err);
	return nullptr;
}

int uuu_run_cmd(const char * cmd, int dry)
{
	return run_cmd(nullptr, cmd, dry);
}

int run_cmd(CmdCtx *pCtx, const char * cmd, int dry)
{
	shared_ptr<CmdBase> p;
	p = create_cmd_obj(cmd);
	int ret;

	if (p == nullptr)
		return -1;

	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_CMD_TOTAL;
	nt.total = 1;
	call_notify(nt);

	nt.type = uuu_notify::NOTIFY_CMD_START;
	nt.str = (char *)p->get_cmd().c_str();
	call_notify(nt);

	if (typeid(*p) != typeid(CfgCmd))
	{
		size_t pos = 0;
		string c = cmd;

		string pro = get_next_param(c, pos, ':');
		pro = remove_square_brackets(pro);
		pro += ":";

		if (p->parser())
			ret = -1;
		else
		{
			if (dry)
			{
				ret = p->dump();
			}else
			{
				CmdUsbCtx ctx;
				if (pCtx == nullptr)
				{
					ret = ctx.look_for_match_device(pro.c_str());
					if (ret)
						return ret;

					pCtx = &ctx;
				}

				ret = p->run(pCtx);
			}
		}
	}
	else
	{
		return ret = dry? p->dump() : p->run(nullptr);
	}

	nt.type = uuu_notify::NOTIFY_CMD_END;
	nt.status = ret;
	call_notify(nt);

	return ret;
}

int CmdDone::run(CmdCtx *)
{
	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_DONE;
	call_notify(nt);
	return 0;
}

int CmdDelay::parser(char * /*p*/)
{
	size_t pos = 0;
	string param = get_next_param(m_cmd, pos);

	if (param.find(':') != string::npos)
		param = get_next_param(m_cmd, pos);

	if (str_to_upper(param) != "DELAY")
	{
		string err = "Unknown Commnd:";
		err += param;
		set_last_err_string(err);
		return -1;
	}

	string ms = get_next_param(m_cmd, pos);
	m_ms = str_to_uint32(ms);
	return 0;
}

int CmdDelay::run(CmdCtx *)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(m_ms));
	return 0;
}

int CmdShell::parser(char * p)
{
	if (p)
		m_cmd = p;

	size_t pos = 0;
	string s;

	if (parser_protocal(p, pos))
		return -1;

	m_protocal = m_cmd.substr(0, pos);

	s = get_next_param(m_cmd, pos);

	m_dyn = (s == "<");

	if (pos != string::npos && pos < m_cmd.size())
		m_shellcmd = m_cmd.substr(pos);

	return 0;
}

int CmdShell::run(CmdCtx*pCtx)
{
#ifndef WIN32
	#define _popen popen
	#define _pclose pclose
#endif
	FILE *pipe = _popen(m_shellcmd.c_str(), "r");

	if (pipe == nullptr)
	{
		string err = "failure popen: ";
		err += m_shellcmd.c_str();
		set_last_err_string(err);
		return -1;
	}

	string str;
	str.resize(256);
	while (fgets((char*)str.c_str(), str.size(), pipe))
	{
		if (m_dyn)
		{
			string cmd;
			cmd = m_protocal;
			str.resize(strlen(str.c_str()));
			cmd += ' ';
			cmd += str;
			
			size_t pos = cmd.find_first_of("\r\n");
			if (pos != string::npos)
				cmd = cmd.substr(0, pos);

			return run_cmd(pCtx, cmd.c_str(), 0);
		}
		uuu_notify nt;
		nt.type = uuu_notify::NOTIFY_CMD_INFO;
		nt.str = (char*)str.c_str();
		call_notify(nt);
	}

	/* Close pipe and print return value of pPipe. */
	if (feof(pipe))
	{
		int ret = _pclose(pipe);
		string_ex str;
		str.format("\nProcess returned %d\n", ret);;
		if (ret)
		{
			set_last_err_string(str.c_str());
			return ret;
		}
	}
	else
	{
		set_last_err_string("Error: Failed to read the end of the pipe.\n");
		return -1;
	}

	return 0;
}

int CmdEnv::parser(char *p)
{
	if (p)
		m_cmd = p;

	size_t pos = 0;

	if (parser_protocal(p, pos))
		return -1;
	if (pos == string::npos || pos >= m_cmd.size())
		return -1;

	m_unfold_cmd = m_cmd.substr(0, pos);
	m_unfold_cmd.append(" ");

	// read the '@'
	get_next_param(m_cmd, pos);

	auto cmd = m_cmd.substr(pos);

	regex expr { "@[0-9a-zA-Z_]+@" };
	smatch result;
	auto last_pos = static_cast<const string&>(cmd).begin();
	auto cmd_end = static_cast<const string&>(cmd).end();
	while (regex_search(last_pos, cmd_end, result, expr)) {
		for (auto &i : result) {
			string key { i.first + 1, i.second - 1 };
			auto value = [&key]() -> pair<bool, string> {
#ifndef WIN32
				auto ptr = getenv(key.c_str());
				if (ptr)
					return {true, ptr};
				return {false, {}};
#else
				size_t len;
				getenv_s(&len, nullptr, 0, key.c_str());
				if (!len)
					return {false, {}};
				string value(len-1, '\0');
				getenv_s(&len, &value[0], len, key.c_str());
				return {true, value};
#endif
			}();
			if (!value.first) {
				set_last_err_string("variable '" + key + "' is not defined");
				return -1;
			}
			auto begin = value.second.begin();
			auto end = value.second.end();
			auto pos = find_if(begin, end, [](char c){ return c == '\r' || c == '\n'; });
			m_unfold_cmd.append(&*last_pos, distance(last_pos, i.first));
			m_unfold_cmd.append(begin, pos);
			last_pos = i.second;
		}
	}
	m_unfold_cmd.append(&*last_pos);

	return 0;
}

int CmdEnv::run(CmdCtx *p)
{
	return run_cmd(p, m_unfold_cmd.c_str(), 0);
}

int run_cmds(const char *procotal, CmdCtx *p)
{
	CmdMap cmdmap, *pCmdMap;

	if (!g_cmd_list_file.empty())
	{
		shared_ptr<FileBuffer> pbuff = get_file_buffer(g_cmd_list_file);
		if (pbuff == nullptr)
			return -1;
		if(parser_cmd_list_file(pbuff, &cmdmap))
			return -1;
		pCmdMap = &cmdmap;
	}
	else
	{
		pCmdMap = &g_cmd_map;
	}

	if (pCmdMap->find(procotal) == pCmdMap->end())
	{
		return 0;
	}

	return (*pCmdMap)[procotal]->run_all(p);
}

static int insert_one_cmd(const char * cmd, CmdMap *pCmdMap)
{
	string s = cmd;
	size_t pos = 0;

	string pro = get_next_param(s, pos, ':');
	pro = remove_square_brackets(pro);
	pro += ":";

	pro = str_to_upper(pro);

	shared_ptr<CmdBase> p = create_cmd_obj(s);
	if (p == nullptr)
		return -1;

	if (p->parser())
		return -1;

	if (pCmdMap->find(pro) == pCmdMap->end())
	{
		shared_ptr<CmdList> list(new CmdList);
		(*pCmdMap)[pro] = list;
	}

	(*pCmdMap)[pro]->push_back(p);

	return 0;
}


static int added_default_boot_cmd(const char *filename)
{
	string str;

	str = "SDPS: boot -f ";
	str += "\"";
	str += filename;
	str += "\"";

	int ret = insert_one_cmd(str.c_str(), &g_cmd_map);
	if (ret) return ret;

	insert_one_cmd("SDPS: done", &g_cmd_map);

	str = "SDP: boot -f ";
	str += "\"";
	str += filename;
	str += "\"";

	ret = insert_one_cmd(str.c_str(), &g_cmd_map);
	if (ret) return ret;

	insert_one_cmd("SDP: done", &g_cmd_map);

	str = "SDPU: write -f ";
	str += "\"";
	str += filename;
	str += "\"";
	str += " -offset 0x57c00";
	insert_one_cmd(str.c_str(), &g_cmd_map);
	insert_one_cmd("SDPU: jump", &g_cmd_map);
	insert_one_cmd("SDPU: done", &g_cmd_map);

	str = "SDPV: write -f ";
	str += "\"";
	str += filename;
	str += "\"";
	str += " -skipspl";
	insert_one_cmd(str.c_str(), &g_cmd_map);
	insert_one_cmd("SDPV: jump", &g_cmd_map);
	insert_one_cmd("SDPV: done", &g_cmd_map);

	return 0;
}

int check_version(string str)
{
	int x = 0;
	int ver = 0;
	for (size_t i = 0; i < str.size(); i++)
	{
		char c = str[i];
		if (c >= '0' && c <= '9')
		{
			x *= 10;
			x += c - '0';
		}
		if (c == '.' || i == str.size()-1 || c == '\n')
		{
			ver <<= 12;
			ver += x;
			x = 0;
		}
	}

	int cur = uuu_get_version();

	if (ver > cur)
	{
		string str;
		str = "This version of uuu is too old, please download the latest one";
		set_last_err_string(str);
		return -1;
	}
	return 0;
}

int uuu_run_cmd_script(const char * buff, int dry)
{
	shared_ptr<FileBuffer> p(new FileBuffer((void*)buff, strlen(buff)));
	
	return parser_cmd_list_file(p);
}

int parser_cmd_list_file(shared_ptr<FileBuffer> pbuff, CmdMap *pCmdMap)
{
	char uuu_version[] = "uuu_version";
	string str;

	if (pCmdMap == nullptr)
		pCmdMap = &g_cmd_map;

	pCmdMap->clear();

	for (size_t i = 0; i < pbuff->size(); i++)
	{
		uint8_t c = pbuff->at(i);
		if (c == '\r')
			continue;

		if(c != '\n')
			str.push_back(c);

		if (c == '\n' || c == 0 || i == pbuff->size() - 1)
		{
			if (str.substr(0, strlen(uuu_version)) == uuu_version)
			{
				if (check_version(str.substr(strlen(uuu_version), 10)))
				{
					return -1;
				}
			}else if (str.size() > 1)
			{
				if (str[0] != '#')
					if (insert_one_cmd(str.c_str(), pCmdMap))
						return -1;
			}
			str.clear();
		}
	}
	return 0;
}

int uuu_auto_detect_file(const char *filename)
{
	string_ex fn;
	fn += remove_quota(filename);
	fn.replace('\\', '/');

	if (fn.empty())
		fn += "./";

	string oldfn =fn;

	fn += "/uuu.auto";
	shared_ptr<FileBuffer> buffer = get_file_buffer(fn);
	if (buffer == nullptr)
	{
		fn.clear();
		fn += oldfn;
		size_t pos = str_to_upper(fn).find("ZIP");
		if(pos == string::npos || pos != fn.size() - 3)
		{
			pos = str_to_upper(fn).find("SDCARD");
			if (pos == string::npos || pos != fn.size() - 6)
				buffer = get_file_buffer(fn); //we don't try open a zip file here
		}

		if(buffer == nullptr)
			return -1;
	}

	string str= "uuu_version";
	void *p1 = buffer->data();
	void *p2 = (void*)str.data();
	if (memcmp(p1, p2, str.size()) == 0)
	{
		size_t pos = fn.rfind('/');
		if (pos != string::npos)
			set_current_dir(fn.substr(0, pos + 1));

		g_cmd_list_file = fn.substr(pos+1);

		return parser_cmd_list_file(buffer);
	}

	//flash.bin or uboot.bin
	return added_default_boot_cmd(fn.c_str());
}

int notify_done(uuu_notify nt, void *p)
{
	if(nt.type == uuu_notify::NOTIFY_DONE)
		*(std::atomic<int> *) p = 1;
	if (nt.type == uuu_notify::NOTIFY_CMD_END && nt.status)
		*(std::atomic<int> *) p = 1;

	return 0;
}
int uuu_wait_uuu_finish(int deamon, int dry)
{
	std::atomic<int> exit;
	exit = 0;

	if(dry) {
		for(auto it=g_cmd_map.begin(); it != g_cmd_map.end(); it++)
		{
			for(auto cmd = it->second->begin(); cmd != it->second->end(); cmd++)
			{
				(*cmd)->dump();
			}
		}
		return 0;
	}

	if (!deamon)
		uuu_register_notify_callback(notify_done, &exit);

	if(polling_usb(exit))
		return -1;

	return 0;
}

