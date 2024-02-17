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

	StatHolder DamageMultiInRoot{StatType::Damage,StatVariant::Mutliplier,98,1.2f};
	StatHolder DamageAddInRoot{StatType::Damage,StatVariant::Flat,99,1.0f};


	float CalculateSoftTears(Entity_Player *player, float t) {
		if (t >= 0) {
			return 16.0f - 6.0f * sqrtf((t * 1.3f) + 1.0f);
		}
		else if (t<0 && t>-0.77) {
			return 16.0f - 6.0f * sqrtf((t * 1.3f) + 1.0f) - 6.0f * t;
		}
		else {
			return 16.0f - 6.0f * t;
		}
	};

	bool CompareHoldersByPriority(const StatHolder& i1, const StatHolder& i2) {
		return (i1.Priority < i2.Priority);
	};

	bool CompareHoldersByVariant(const StatHolder& i1, const StatHolder& i2) {
		return (i1.Variant < i2.Variant);
	};

	class StatsManager {
	public:
		std::unordered_map<int, std::vector<StatHolder>> ItemsList;

		int ParseStatsJSON(std::string const& filepath);
		void EvaluateAllPlayerStats();
		void PopulateStatsHolder();
		bool Enabled = true;
		bool DataLoaded = false;
	};
}

#endif