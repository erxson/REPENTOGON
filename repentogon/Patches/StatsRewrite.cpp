#include "SigScan.h"
#include "IsaacRepentance.h"
#include "HookSystem.h"

#include "../ImGuiFeatures/LogViewer.h"
#include "StatsRewrite.h"

void StatsRewrite::PopulateStatsHolder() {
	//todo: the actual damn thing, there is no stats holder yet
	g_Game->GetConsole()->Print("Hello!\n", 0xFFFFFF, 0);
}

StatsRewrite stats;

HOOK_METHOD(Entity_Player, EvaluateItems, () -> void) {
	if (!stats.Enabled) {
		super();
		return;
	}

	this->_damage = 9001.0;		//to ensure that the hook works :)
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
			printf("[REPENTOGON]"" stats: %p\n", &stats);
			printf("[REPENTOGON]"" stats state: %d\n", stats.Enabled);
			stats.Enabled = !stats.Enabled;
			printf("[REPENTOGON]"" stats state: %d\n", stats.Enabled);
			if (stats.Enabled) {
				g_Game->GetConsole()->Print("Stats rewrite enabled!\n", 0xFFFFFF, 0);
			}
			else {
				g_Game->GetConsole()->Print("Stats rewrite disabled!\n", 0xFFFFFF, 0);
			}
			printf("[REPENTOGON]""end is here!\n");
		}

		if (cmdlets[1].rfind("reeval",0)==0) {
			printf("[REPENTOGON]"" player reeval begin!\n");
			std::vector<Entity_Player*> PlayerList=g_Game->GetPlayerManager()->_playerList;
			printf("[REPENTOGON]"" player list size: %d\n", PlayerList.size());
			for (size_t i = 0; i < PlayerList.size(); i++) {
				Entity_Player* playerPtr = PlayerList[i];
				printf("[REPENTOGON]"" player ptr: %p\n", playerPtr);
				(playerPtr)->AddCacheFlags((1 << 16) - 1);	//CacheFlag.CACHE_ALL
				(playerPtr)->EvaluateItems();
			}
		}

		for (auto it = cmdlets.begin(); it != cmdlets.end(); ++it) {
			logViewer.AddLog("[REPENTOGON]", "iter: %s\n", it);
		}
		
	}
	super(in, out, player);
}