#include "SigScan.h"
#include "IsaacRepentance.h"
#include "HookSystem.h"
#include "rapidjson.h"
#include "../rapidjson/document.h"
#include <fstream>
#include <cstring>
#include <cmath>

#include "../ImGuiFeatures/LogViewer.h"
#include "StatsRewrite.h"


void StatsRewrite::StatsManager::EvaluateAllPlayerStats() {
	std::vector<Entity_Player*> PlayerList = g_Game->GetPlayerManager()->_playerList;
	for (size_t i = 0; i < PlayerList.size(); i++) {
		Entity_Player* playerPtr = PlayerList[i];
		(playerPtr)->AddCacheFlags((1 << 16) - 1);	//CacheFlag.CACHE_ALL
		(playerPtr)->EvaluateItems();
	};
};

int StatsRewrite::StatsManager::ParseStatsJSON(std::string const& filepath) {
	std::ifstream json_file;
	std::string json_data;
	rapidjson::Document document;
	int current_item_id = 0;
	const char* cstr_holder;
	std::vector<StatsRewrite::StatHolder>* CurrentObject;

	json_file.open(filepath, std::ios::out);
	if (!json_file.is_open()) {
		printf("Failed to open Stats JSON!\n");
		return 1;
	}
	std::ostringstream bufferstream;
	bufferstream << json_file.rdbuf();
	json_data = bufferstream.str();

	document.Parse(json_data.c_str());
	json_file.close();

	if (document.HasParseError()) {
		printf("JSON Parse Error!\n");
		return 2;
	}

	DataLoaded = 1;

	if (document.HasMember("items")) {
		for (rapidjson::GenericMemberIterator item_iter = document["items"].MemberBegin(); item_iter != document["items"].MemberEnd(); item_iter++) {
			printf("item iter: found member %s\n", item_iter->name.GetString());
			current_item_id = atoi(item_iter->name.GetString());
			ItemsList[current_item_id] = std::vector<StatsRewrite::StatHolder>();
			CurrentObject = &ItemsList[current_item_id];
			for (size_t prop = 0; prop < (item_iter->value.Size()); prop++) {
				printf("item iter: property number %d\n", prop);
				StatsRewrite::StatHolder current_stat;
				if (item_iter->value[prop].HasMember("stat")) {				//todo: add all types here!!
					cstr_holder = item_iter->value[prop]["stat"].GetString();
					printf("current stat is %s\n", cstr_holder);
					if (strcmp(cstr_holder, "damage") == 0) {	
						current_stat.Type = StatsRewrite::StatType::Damage;
					}
					else if (strcmp(cstr_holder, "tears") == 0) {
						current_stat.Type = StatsRewrite::StatType::Tears;
					}
					else if (strcmp(cstr_holder, "movespeed") == 0) {
						current_stat.Type = StatsRewrite::StatType::MoveSpeed;
					}
				}
				if (item_iter->value[prop].HasMember("priority")) {
					current_stat.Priority = item_iter->value[prop]["priority"].GetInt();
				}

				if (item_iter->value[prop].HasMember("value")) {
					current_stat.Value = item_iter->value[prop]["value"].GetFloat();
				}

				if (item_iter->value[prop].HasMember("variant")) {
					cstr_holder = item_iter->value[prop]["variant"].GetString();
					if (strcmp(cstr_holder, "flat") == 0) {
						current_stat.Variant = StatsRewrite::StatVariant::Flat;
					}
					else if (strcmp(cstr_holder, "multi") == 0) {
						current_stat.Variant = StatsRewrite::StatVariant::Mutliplier;
					}
				}

				CurrentObject->push_back(current_stat);
			}
		}
	}

	return 0;
}

void StatsRewrite::StatsManager::PopulateStatsHolder() {
	std::string jsonpath = "stats.json";
	ItemsList.clear();
	ParseStatsJSON(jsonpath);
	EvaluateAllPlayerStats();
}

StatsRewrite::StatsManager stats;
HOOK_METHOD(Entity_Player, EvaluateItems, () -> void) {
	if (!stats.DataLoaded) {
		stats.PopulateStatsHolder();

	}
	if (!stats.Enabled) {
		super();
		return;
	}
	int ItemCount = 0;
	int CurrentPriority = 0;
	float DamageUnderRoot = 0.0;
	float SoftTears = 0.0;
	bool DamageSqrtApplied = false;
	bool SoftTearsApplied = false;

	float *PStatToAlter=nullptr;
	PStatToAlter = &DamageUnderRoot;
	

	this->_damage = 3.5f;
	this->_maxfiredelay = 10.0f;

	std::vector<StatsRewrite::StatHolder> FullStatsList;

	FullStatsList.push_back(StatsRewrite::DamageMultiInRoot);		//the mandatory ones to make damage values match the vanilla
	FullStatsList.push_back(StatsRewrite::DamageAddInRoot);

	int AmountToAdd = 0;
	//todo: add other sources of stats e.g. coll effects, char start stats, etc...
	for (int cidx = 0; cidx < this->GetCollectiblesList().size(); cidx++) {
		ItemCount = this->GetCollectiblesList()[cidx];
		if (stats.ItemsList.find(cidx) != stats.ItemsList.end()) {
			if (ItemCount) {
				for (size_t stat_idx = 0; stat_idx < stats.ItemsList[cidx].size(); stat_idx++) {
					StatsRewrite::StatHolder& stat = stats.ItemsList[cidx][stat_idx];
					AmountToAdd = 1 + (ItemCount - 1) * stat.ShouldStack;		//todo: split damage into two types, poison and no poison
					for (int i = 0; i < AmountToAdd;i++) {
						FullStatsList.push_back(stat);
					}
				}
			}
		}
	}


	std::stable_sort(FullStatsList.begin(), FullStatsList.end(), StatsRewrite::CompareHoldersByVariant);	//multi after flat
	std::stable_sort(FullStatsList.begin(), FullStatsList.end(),StatsRewrite::CompareHoldersByPriority);	//asc order


	for (StatsRewrite::StatHolder& stat : FullStatsList) {
		switch (stat.Type) {
		case StatsRewrite::StatType::Damage:
			if (stat.Priority < 101) {	//hey, tem, you didn't forget
				PStatToAlter = &DamageUnderRoot;
			}
			else {
				if (!DamageSqrtApplied) {
					this->_damage = this->_damage * sqrtf(DamageUnderRoot);
					DamageSqrtApplied = true;
				}
				PStatToAlter = &(this->_damage);
			}
			break;
		case StatsRewrite::StatType::Tears:
			if (stat.Priority < 101) {
				PStatToAlter = &SoftTears;
			}
			else {
				if (!SoftTearsApplied) {
					this->_maxfiredelay = StatsRewrite::CalculateSoftTears(this, SoftTears);
					SoftTearsApplied = true;
				}
				PStatToAlter = &(this->_maxfiredelay);
			}
			break;
		default:
			PStatToAlter = nullptr;	//todo: get other stats working!
			break;
		}

		if (PStatToAlter == nullptr) {
			continue;
		}

		if (stat.Variant == StatsRewrite::StatVariant::Flat) {
			*(PStatToAlter) += stat.Value;
		}
		else if (stat.Variant == StatsRewrite::StatVariant::Mutliplier) {
			*(PStatToAlter) *= stat.Value;
		}
	}
	if (!DamageSqrtApplied) {
		this->_damage = this->_damage * sqrtf(DamageUnderRoot);
		DamageSqrtApplied = true;
	}
	if (!SoftTearsApplied) {
		this->_maxfiredelay = StatsRewrite::CalculateSoftTears(this,SoftTears);
		SoftTearsApplied = true;
	}
	logViewer.AddLog("[REPENTOGON]", "Hello from the stat rewrite!\n");
	return;
}


static std::vector<std::string> ParseCommand2(const std::string& command, int size = 0) {
	std::vector<std::string> cmdlets;

	std::stringstream sstream(command);
	std::string cmdlet;
	char space = ' ';
	while (std::getline(sstream, cmdlet, space)) {
		cmdlet.erase(std::remove_if(cmdlet.begin(), cmdlet.end(), ispunct), cmdlet.end());
		cmdlets.push_back(cmdlet);
		if (size > 0 && cmdlets.size() == size) {
			break;
		}
	}
	return cmdlets;
}

HOOK_METHOD(Console, RunCommand, (std_string& in, std_string* out, Entity_Player* player)-> void) {
	if (in.rfind("stats", 0) == 0) {
		std::vector<std::string> cmdlets = ParseCommand2(in, 0);
		if (cmdlets.size() == 1) {
			g_Game->GetConsole()->Print("Usage:\n"
				"\"stats reload\" to load the stats from a file\n"
				"\"stats toggle\" to toggle the impl on/off\n"
				"\"stats reeval\" to reevaluate stats\n", 0xFFFFFF, 0);
			return;
		}

		if (cmdlets[1].rfind("reload",0)==0) {
			g_Game->GetConsole()->Print("Should reload now!\n", 0xFFFFFF, 0);
			stats.PopulateStatsHolder();
		}

		if (cmdlets[1].rfind("toggle",0)==0) {
			stats.Enabled = !stats.Enabled;
			if (stats.Enabled) {
				g_Game->GetConsole()->Print("Stats rewrite enabled!\n", 0xFFFFFF, 0);
			}
			else {
				g_Game->GetConsole()->Print("Stats rewrite disabled!\n", 0xFFFFFF, 0);
			}
		}

		if (cmdlets[1].rfind("reeval",0)==0) {
			stats.EvaluateAllPlayerStats();
		}
	}
	super(in, out, player);
}