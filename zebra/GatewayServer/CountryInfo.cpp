#include "GatewayServer.h"


/**
* \brief 根据国家id得到国家信息
*
*
* \param country_id: 国家id
* \return 国家信息
*/
CountryInfo::Info *CountryInfo::getInfo(DWORD country_id)
{
	for(StrVec_iterator iter = country_info.begin() ; iter != country_info.end() ; iter ++)
	{
		if (iter->countryid == country_id)
		{
			return &*iter;
		}
	}
	return NULL;
}
/**
* \brief 从配置文件中读取国家信息
*
* \return 读取是否成功
*/
bool CountryInfo::reload()
{
	zXMLParser parser;
	if (parser.initFile(Zebra::global["confdir"] + "scenesinfo.xml"))
	{
		xmlNodePtr root=parser.getRootNode("ScenesInfo");
		xmlNodePtr countryNode=parser.getChildNode(root,"countryinfo");
		if (countryNode)
		{
			xmlNodePtr subnode = parser.getChildNode(countryNode,"country");
			while(subnode)
			{
				if (strcmp((char*)subnode->name,"country") == 0)
				{
					CountryDic info;
					bzero(&info,sizeof(info));
					parser.getNodePropNum(subnode,"id",&info.id,sizeof(info.id));
					parser.getNodePropStr(subnode,"name",info.name,sizeof(info.name));
					parser.getNodePropNum(subnode,"mapID",&info.mapid,sizeof(info.mapid));
					parser.getNodePropNum(subnode,"function",&info.function,sizeof(info.function));
					Zebra::logger->info("重新读取国家名称(%d,%s,%d,%d)",info.id,info.name,info.mapid,info.function);
					CountryMap_iter iter = country_dic.find(info.id);
					if (iter !=country_dic.end())
					{
						Zebra::logger->debug("reload国家字典成功%s",info.name);
						iter->second.function=info.function;
					}
					for(StrVec_iterator iter_1 = country_info.begin() ; iter_1 != country_info.end() ; iter_1 ++)
					{
						if (info.id == (*iter_1).countryid)
						{
							Zebra::logger->debug("reload国家信息成功%s",info.name);
							(*iter_1).function=info.function;
							break;
						}
					}
				}
				subnode = parser.getNextNode(subnode,NULL);
			}
		}
	}
	return true;
}
/**
* \brief 从配置文件中读取国家信息
*
* \return 读取是否成功
*/
bool CountryInfo::init()
{
	bool inited = false;
	zXMLParser parser;

	if (parser.initFile(Zebra::global["confdir"] + "scenesinfo.xml"))
	{
		xmlNodePtr root=parser.getRootNode("ScenesInfo");
		xmlNodePtr mapNode=parser.getChildNode(root,"mapinfo");
		if (mapNode)
		{
			xmlNodePtr subnode = parser.getChildNode(mapNode,"map");
			while(subnode)
			{
				if (strcmp((char*)subnode->name,"map") == 0)
				{
					MapDic info;
					bzero(&info,sizeof(info));
					parser.getNodePropNum(subnode,"mapID",&info.id,sizeof(info.id));
					parser.getNodePropStr(subnode,"name",info.name,sizeof(info.name));
					parser.getNodePropStr(subnode,"fileName",info.filename,sizeof(info.filename));
					parser.getNodePropNum(subnode,"backto",&info.backto,sizeof(info.backto));
					Zebra::logger->info("加载地图名称(%d,%s,%s,%d)",info.id,info.name,info.filename,info.backto);
					map_dic.insert(MapMap_value_type(info.id,info));
				}
				subnode = parser.getNextNode(subnode,NULL);
			}
		}
		xmlNodePtr countryNode=parser.getChildNode(root,"countryinfo");
		if (countryNode)
		{
			country_info.clear();
			xmlNodePtr subnode = parser.getChildNode(countryNode,"country");
			while(subnode)
			{
				if (strcmp((char*)subnode->name,"country") == 0)
				{
					CountryDic info;
					bzero(&info,sizeof(info));
					parser.getNodePropNum(subnode,"id",&info.id,sizeof(info.id));
					parser.getNodePropStr(subnode,"name",info.name,sizeof(info.name));
					parser.getNodePropNum(subnode,"mapID",&info.mapid,sizeof(info.mapid));
					parser.getNodePropNum(subnode,"function",&info.function,sizeof(info.function));
					parser.getNodePropNum(subnode,"type",&info.type,sizeof(info.type));
					Zebra::logger->info("加载国家名称(%d,%s,%d,%d,%d)",info.id,info.name,info.mapid,info.function,info.type);
					country_dic.insert(CountryMap_value_type(info.id,info));
					Info info_1;
					info_1.countryid = info.id;
					info_1.countryname = info.name;
					info_1.function = info.function;
					info_1.type     = info.type;
					if (info.mapid)
					{
						MapMap_iter map_iter = map_dic.find(info.mapid);
						if (map_iter == map_dic.end())
						{
							Zebra::logger->error("得到%d地图名称失败",info.mapid);
							continue;
						}
						inited = true;
						info_1.mapname = info_1.countryname + "·" + map_iter->second.name;
					}
					country_info.push_back(info_1);
				}
				subnode = parser.getNextNode(subnode,NULL);
			}
		}
	}
	if (inited)
	{
		for(StrVec_iterator iter = country_info.begin() ; iter != country_info.end() ; iter++)
		{
			Zebra::logger->info("读取国家信息:%s(%d),%s",
				(*iter).countryname.c_str(),(*iter).countryid,(*iter).mapname.c_str());
		}
	}
	return inited;
}

/**
* \brief 设置国家排序
*
* \param ptCmd 排序指令
*/
void CountryInfo::setCountryOrder(Cmd::Session::CountrOrder *ptCmd)
{
	mutex.lock(); 
	bzero(country_order,sizeof(country_order));
	for(int i = 0 ; i < (int)ptCmd->size ; i++)
	{
		country_order[i]=ptCmd->order[i].country;
	}
	mutex.unlock(); 
}
// [ranqd Add] 获取国家在线状态
int CountryInfo::getCountryState( CountryInfo::Info cInfo )
{
	DWORD now = cInfo.Online_Now;
	DWORD max = cInfo.Online_Max;
	if( max == 0 )				return STATE_SERVICING;
	if( now == max + 10000 )	return STATE_SERVICING;
	if( now < max * 0.7 )		return STATE_NOMARL;
	if( now < max * 0.9 )		return STATE_GOOD;
	if( now < max )				return STATE_BUSY;
	if( now >= max )			return STATE_FULL;
}
/**
* \brief 得到所有国家名称
*
*
* \param buf:输出国家名称的buf
* \return 读取到的国家数量
*/
// [ranqd] 最大有效国家ID，100以上为战场使用
#define MAX_VALUE_COUNTRYID 100

int CountryInfo::getAll(char *buf)
{
	int size = 0;
	if (!buf)
	{
		return size;
	}
	mutex.lock(); 
	Cmd::Country_Info *info = (Cmd::Country_Info*)buf;
	for(StrVec_iterator iter = country_info.begin() ; iter != country_info.end() ; iter++)
	{
		bool ok=true;
		for(int i = 0; country_order[i] != 0; i ++)
		{
			if ((*iter).countryid == country_order[i])
			{
				ok=false;
				break;
			}
		}
		if (ok && (*iter).countryid < MAX_VALUE_COUNTRYID )
		{
			//如果这个国家可以注册
			//if (!((*iter).function & 0x1))
			//{
			info[size].id = (*iter).countryid;
			info[size].enableLogin = ((*iter).function & 0x2)==0?1:0;
			info[size].enableRegister = ((*iter).function & 0x1)==0?1:0;
			info[size].Online_Statue  = getCountryState((*iter));
			info[size].type           = (*iter).type;
			bcopy((*iter).countryname.c_str(),info[size].pstrName,MAX_NAMESIZE,sizeof(info[size].pstrName));
			size++;
			//}
		}
	}
	for(int i = 0; country_order[i] != 0; i ++)
	{
		for(StrVec_iterator iter = country_info.begin() ; iter != country_info.end() ; iter++)
		{
			if ((*iter).countryid < MAX_VALUE_COUNTRYID && (*iter).countryid == country_order[i])
			{
				//如果这个国家可以注册
				//if (!((*iter).function & 0x1))
				//{
				info[size].id = (*iter).countryid;
				info[size].enableLogin = ((*iter).function & 0x2)==0?1:0;
				info[size].enableRegister = ((*iter).function & 0x1)==0?1:0;
				info[size].Online_Statue  = getCountryState((*iter));
				info[size].type           = (*iter).type;
				bcopy((*iter).countryname.c_str(),info[size].pstrName,MAX_NAMESIZE,sizeof(info[size].pstrName));
				size++;
				break;
				//}
			}
		}
	}
	mutex.unlock(); 
	return size;
}
/**
* \brief 得到国家数量
*
* \return 国家总数
*/
int CountryInfo::getCountrySize()
{
	return country_info.size();
}
/**
* \brief 根据hash后的mapid得到配置文件中本身的mapid
*
* \param map_id: hash后的mapid
* \return 配置文件中的mapid
*/
DWORD CountryInfo::getRealMapID(DWORD map_id)
{
	return map_id & 0x0000FFFF;
}
/**
* \brief 更具hash后的地图名称得到配置文件中的地图名称
*
* \param name: hash后的地图名称
* \return 配置文件中的地图名称
*/
const char *CountryInfo::getRealMapName(const char *name)
{
	char *real = const_cast<char*>(strstr(name,"·"));
	if (real != NULL)
	{
		return real + 2;
	}
	else
	{
		return name;
	}
}
/**
* \brief 检查国家id是否合法
*
*
* \param country_id:国家id
* \return 如果存在该国家id返回国家id,否则返回-1
*/
DWORD CountryInfo::getCountryID(DWORD country_id)
{
	Info *info = getInfo(country_id);
	if (info)
	{
		return info->countryid;
	}
	return (DWORD)-1;
}
/**
* \brief 根据国家id得到国家名称
*
*
* \param country_id:国家id
* \return 找到返回国家名称否则返回""
*/
std::string CountryInfo::getCountryName(DWORD country_id)
{
	Info *info = getInfo(country_id);
	if (info)
	{
		return info->countryname;
	}
	return "";
}
/**
* \brief 更具国家id得到该国家出生地map的名称
*
*
* \param country_id:国家名称
* \return 出生地地图名称
*/
std::string CountryInfo::getMapName(DWORD country_id)
{
	Info *info = getInfo(country_id);
	if (info)
	{
		return info->mapname;
	}
	return "";
}

/**
* \brief 根据国家id得到该国家是否允许登陆
* \param country_id:国家名称
* \return true允许 false不允许
*/
bool CountryInfo::isEnableLogin(DWORD country_id)
{
	Info *info = getInfo(country_id);
	if (info)
	{
		return !(info->function&0x2);
	}
	return false;
}

/**
* \brief 根据国家id得到该国家是否允许注册
* \param country_id:国家名称
* \return true允许 false不允许
*/
bool CountryInfo::isEnableRegister(DWORD country_id)
{
	Info *info = getInfo(country_id);
	if (info)
	{
		return !(info->function&0x1);
	}
	return false;
}

/**
* \brief 根据国家ID处理对国家function的状态变更
* \param country_id:国家名称
*/
void CountryInfo::processChange(GateUser *pUser,Cmd::Scene::t_ChangeCountryStatus *rev)
{
	if (!rev||!pUser) return;
	switch(rev->oper)
	{
	case Cmd::Scene::ENABLE_REGISTER:
		{
			Info *info = getInfo(rev->country);
			if (info)
			{
				info->function = info->function&(~0x1);
			}
		}
		break;
	case Cmd::Scene::DISABLE_REGISTER:
		{
			Info *info = getInfo(rev->country);
			if (info)
			{
				info->function = info->function|0x1;
			}
		}
		break;
	case Cmd::Scene::ENABLE_LOGIN:
		{
			Info *info = getInfo(rev->country);
			if (info)
			{
				info->function = info->function&(~0x2);
			}
		}
		break;
	case Cmd::Scene::DISABLE_LOGIN:
		{
			Info *info = getInfo(rev->country);
			if (info)
			{
				info->function = info->function|0x2;
			}
		}
		break;
	case Cmd::Scene::LIST_COUNTRY_INFO:
		{
			for(StrVec_iterator iter = country_info.begin() ; iter != country_info.end() ; iter++)
			{
				Cmd::stChannelChatUserCmd send;
				send.dwType=Cmd::CHAT_TYPE_SYSTEM;
				send.dwSysInfoType = Cmd::INFO_TYPE_GAME;
				bzero(send.pstrName,sizeof(send.pstrName));
				bzero(send.pstrChat,sizeof(send.pstrChat));

				if (pUser)
				{
					sprintf((char*)send.pstrChat,"国家:%s ID:%d 注册:%s 登录:%s",
						(*iter).countryname.c_str(),(*iter).countryid,(((*iter).function&0x1)==0)?"开":"关",(((*iter).function&0x2)==0)?"开":"关");
					pUser->sendCmd(&send,sizeof(send));
				}
			}
		}
		break;
	default:
		break;
	}
}
// [ranqd] 更新国家在线状态
void CountryInfo::UpdateCountryOnline( DWORD country_id, DWORD online_numbers )
{
	for(StrVec_iterator iter = country_info.begin() ; iter != country_info.end() ; iter++)
	{
		if( (*iter).countryid == country_id )
		{
			(*iter).Online_Now = online_numbers;
			return;
		}
	}
}