#ifndef STATSREWRITE_H
#define STATSREWRITE_H

namespace StatsRewrite {
	enum class StatType {
		None, Damage, Tears, TearRange, TearHeight, MoveSpeed, ShotSpeed, CanFly
	};
	enum class StatVariant {
		Flat, Mutliplier
	};

	struct StatHolder {
		StatType Type = StatType::None;
		StatVariant Variant = StatVariant::Flat;
		int Priority = 0;
		float Value = 0.0;
		bool ShouldStack = true;

	};

	class StatsManager {
	public:
		std::unordered_map<int, std::vector<StatHolder>> ItemsList;

		int ParseStatsJSON(std::string const& filepath);
		void EvaluateAllPlayerStats();
		void PopulateStatsHolder();
		bool Enabled = 1;
		bool DataLoaded = 0;
	};
}

#endif