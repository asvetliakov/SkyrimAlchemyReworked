#pragma once

#include <SKSE/SKSE.h>
#include <articuno/articuno.h>

class Debug {
public:
    [[nodiscard]] inline spdlog::level::level_enum GetLogLevel() const noexcept { return _logLevel; }

    [[nodiscard]] inline spdlog::level::level_enum GetFlushLevel() const noexcept { return _flushLevel; }

private:
    articuno_serialize(ar) {
        auto logLevel = spdlog::level::to_string_view(_logLevel);
        auto flushLevel = spdlog::level::to_string_view(_flushLevel);
        ar <=> articuno::kv(logLevel, "logLevel");
        ar <=> articuno::kv(flushLevel, "flushLevel");
    }

    articuno_deserialize(ar) {
        *this = Debug();
        std::string logLevel;
        std::string flushLevel;
        if (ar <=> articuno::kv(logLevel, "logLevel")) {
            _logLevel = spdlog::level::from_str(logLevel);
        }
        if (ar <=> articuno::kv(flushLevel, "flushLevel")) {
            _flushLevel = spdlog::level::from_str(flushLevel);
        }
    }

    spdlog::level::level_enum _logLevel{spdlog::level::level_enum::info};
    spdlog::level::level_enum _flushLevel{spdlog::level::level_enum::trace};

    friend class articuno::access;
};

class CobjConfig {
public:
    // Not initialized by default since it would revert to def value if empty in config (can be intended someone might
    // want to disable few levels)
    std::string level1Recipe;     // = "common|common";
    std::string level2Recipe;     // = "common|uncommon";
    std::string level3Recipe;     // = "common|rare";
    std::string level3RecipeAlt;  // = "uncommon|uncommon";
    std::string level4Recipe;     // = "uncommon|rare";
    std::string level5Recipe;     // = "rare|rare";

private:
    articuno_serialize(ar) {
        ar <=> articuno::kv(level1Recipe, "level1");
        ar <=> articuno::kv(level2Recipe, "level2");
        ar <=> articuno::kv(level3Recipe, "level3");
        ar <=> articuno::kv(level3RecipeAlt, "level3Alt");
        ar <=> articuno::kv(level4Recipe, "level4");
        ar <=> articuno::kv(level5Recipe, "level5");
    }

    articuno_deserialize(ar) {
        *this = CobjConfig();
        std::string _level1Recipe;
        std::string _level2Recipe;
        std::string _level3Recipe;
        std::string _level3RecipeAlt;
        std::string _level4Recipe;
        std::string _level5Recipe;

        if (ar <=> articuno::kv(_level1Recipe, "level1")) {
            level1Recipe = _level1Recipe;
        }
        if (ar <=> articuno::kv(_level2Recipe, "level2")) {
            level2Recipe = _level2Recipe;
        }
        if (ar <=> articuno::kv(_level3Recipe, "level3")) {
            level3Recipe = _level3Recipe;
        }
        if (ar <=> articuno::kv(_level3RecipeAlt, "level3Alt")) {
            level3RecipeAlt = _level3RecipeAlt;
        }
        if (ar <=> articuno::kv(_level4Recipe, "level4")) {
            level4Recipe = _level4Recipe;
        }
        if (ar <=> articuno::kv(_level5Recipe, "level5")) {
            level5Recipe = _level5Recipe;
        }
    }
    friend class articuno::access;
};

class IngredientsConfig {
public:
    bool renameIngredients = true;
    std::string commonSuffix = "(Common)";
    std::string uncommonSuffix = "(Uncommon)";
    std::string rareSuffix = "(Rare)";

private:
    articuno_serialize(ar) {
        ar <=> articuno::kv(renameIngredients, "addRaritySuffix");
        ar <=> articuno::kv(commonSuffix, "commonSuffix");
        ar <=> articuno::kv(uncommonSuffix, "uncommonSuffix");
        ar <=> articuno::kv(rareSuffix, "rareSuffix");
    }

    articuno_deserialize(ar) {
        *this = IngredientsConfig();
        std::string _renameIngr;
        std::string _commonSuffix;
        std::string _uncommonSuffix;
        std::string _rareSuffix;

        if (ar <=> articuno::kv(_renameIngr, "addRarirySuffix")) {
            renameIngredients = _renameIngr == "true" || _renameIngr == "1";
        }
        if (ar <=> articuno::kv(_commonSuffix, "commonSuffix")) {
            commonSuffix = _commonSuffix;
        }
        if (ar <=> articuno::kv(_uncommonSuffix, "uncommonSuffix")) {
            uncommonSuffix = _uncommonSuffix;
        }
        if (ar <=> articuno::kv(_rareSuffix, "rareSuffix")) {
            rareSuffix = _rareSuffix;
        }
    }
    friend class articuno::access;
};

class PerksConfig {
public:
    std::string level2Perk;
    std::string level3Perk;
    std::string level4Perk;
    std::string level5Perk;
    std::string potionQualityPerk;
    std::string poisonQualityPerk;
    std::string allQualityPerk;
    std::string doubleItemsPerk;

private:
    articuno_serialize(ar) {
        ar <=> articuno::kv(level2Perk, "level2");
        ar <=> articuno::kv(level3Perk, "level3");
        ar <=> articuno::kv(level4Perk, "level4");
        ar <=> articuno::kv(level5Perk, "level5");
        ar <=> articuno::kv(potionQualityPerk, "potionQuality");
        ar <=> articuno::kv(poisonQualityPerk, "poisonQuality");
        ar <=> articuno::kv(allQualityPerk, "allQuality");
        ar <=> articuno::kv(doubleItemsPerk, "doubleItems");
    }

    articuno_deserialize(ar) {
        *this = PerksConfig();
        std::string _level2Perk;
        std::string _level3Perk;
        std::string _level4Perk;
        std::string _level5Perk;
        std::string _potionQualityPerk;
        std::string _poisonQualityPerk;
        std::string _allQualityPerk;
        std::string _doubleItemsPerk;

        if (ar <=> articuno::kv(_level2Perk, "level2")) {
            level2Perk = _level2Perk;
        }
        if (ar <=> articuno::kv(_level3Perk, "level3")) {
            level3Perk = _level3Perk;
        }
        if (ar <=> articuno::kv(_level4Perk, "level4")) {
            level4Perk = _level4Perk;
        }
        if (ar <=> articuno::kv(_level5Perk, "level5")) {
            level5Perk = _level5Perk;
        }
        if (ar <=> articuno::kv(_potionQualityPerk, "potionQuality")) {
            potionQualityPerk = _potionQualityPerk;
        }
        if (ar <=> articuno::kv(_poisonQualityPerk, "poisonQuality")) {
            poisonQualityPerk = _poisonQualityPerk;
        }
        if (ar <=> articuno::kv(_allQualityPerk, "allQuality")) {
            allQualityPerk = _allQualityPerk;
        }
        if (ar <=> articuno::kv(_doubleItemsPerk, "doubleItems")) {
            doubleItemsPerk = _doubleItemsPerk;
        }
    }

    friend class articuno::access;
};

class Config {
public:
    [[nodiscard]] inline const Debug& GetDebug() const noexcept { return _debug; }
    [[nodiscard]] inline const PerksConfig& GetPerksConfig() const noexcept { return _perks_config; }
    [[nodiscard]] inline const IngredientsConfig& GetIngrConfig() const noexcept { return _ingr_config; }
    [[nodiscard]] inline const CobjConfig& GetCobjConfig() const noexcept { return _cobj_config; }

    [[nodiscard]] static const Config& GetSingleton() noexcept;

private:
    articuno_serde(ar) {
        ar <=> articuno::kv(_debug, "debug");
        ar <=> articuno::kv(_perks_config, "perks");
        ar <=> articuno::kv(_ingr_config, "ingredients");
        ar <=> articuno::kv(_cobj_config, "crafting");
    }

    Debug _debug;
    PerksConfig _perks_config;
    IngredientsConfig _ingr_config;
    CobjConfig _cobj_config;

    friend class articuno::access;
};
