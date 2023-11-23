#include "Distributor.h"

#include <ranges>

#include "Config.h"

using namespace RE;
using namespace RE::BSScript;
using namespace REL;
using namespace SKSE;

namespace {

    struct EffectPotions {
        AlchemyItem* level1;
        AlchemyItem* level2;
        AlchemyItem* level3;
        AlchemyItem* level4;
        AlchemyItem* level5;
    };

    struct CobjMetadata {
        int targetIngredientLevel;
        int potionMinLevel;
        IngredientItem* ingr1;
        IngredientItem* ingr2;
    };

    inline BGSKeyword* alchemyKeyword;
    inline BGSKeyword* level1Keyword;
    inline BGSKeyword* level2Keyword;
    inline BGSKeyword* level3Keyword;
    inline BGSKeyword* level4Keyword;
    inline BGSKeyword* level5Keyword;
    inline std::map<EffectSetting*, EffectPotions> potionsByEffect = {};
    inline std::map<BGSConstructibleObject*, CobjMetadata> constructibleMetadata = {};

    inline std::vector<IngredientItem*> commonIngredients = {};
    inline std::vector<IngredientItem*> uncommonIngredients = {};
    inline std::vector<IngredientItem*> rareIngredients = {};

    inline BGSPerk* level2Perk;
    inline BGSPerk* level3Perk;
    inline BGSPerk* level4Perk;
    inline BGSPerk* level5Perk;
    inline BGSPerk* potionQualityPerk;
    inline BGSPerk* poisonQualityPerk;
    inline BGSPerk* allQualityPerk;
    inline BGSPerk* doubleItemsPerk;

    inline BGSPerk* LoadPerkFromConfig(std::string str) {
        log::info("Loading {}", str);
        int delimterIndex = str.find("|");
        if (delimterIndex < 0) {
            return nullptr;
        }
        auto modFile = str.substr(0, delimterIndex);
        auto formIdStr = str.substr(delimterIndex + 1);
        log::info("{} {}", modFile, formIdStr);
        if (modFile == "" || formIdStr == "") {
            return nullptr;
        }
        unsigned int formId;
        std::stringstream ss;
        ss << std::hex << formIdStr;
        ss >> formId;
        return TESDataHandler::GetSingleton()->LookupForm<BGSPerk>(formId, modFile);
    }

    inline AlchemyItem* GetEarliestLevelPotion(EffectPotions& potions, int* outLevel) {
        if (potions.level1) {
            *outLevel = 1;
            return potions.level1;
        } else if (potions.level2) {
            *outLevel = 2;
            return potions.level2;
        } else if (potions.level3) {
            *outLevel = 3;
            return potions.level3;
        } else if (potions.level4) {
            *outLevel = 4;
            return potions.level4;
        } else if (potions.level5) {
            *outLevel = 5;
            return potions.level5;
        }
        return nullptr;
    }

    inline AlchemyItem* GetHigherLevelPotion(EffectPotions& potions, int targetLevel) {
        if (targetLevel <= 1) {
            return potions.level1;
        }
        if (targetLevel == 2) {
            if (potions.level2) return potions.level2;
            return potions.level1;
        }
        if (targetLevel == 3) {
            if (potions.level3) return potions.level3;
            if (potions.level2) return potions.level2;
            return potions.level1;
        }
        if (targetLevel == 4) {
            if (potions.level4) return potions.level4;
            if (potions.level3) return potions.level3;
            if (potions.level2) return potions.level2;
            return potions.level1;
        }
        if (targetLevel >= 5) {
            if (potions.level5) return potions.level5;
            if (potions.level4) return potions.level4;
            if (potions.level3) return potions.level3;
            if (potions.level2) return potions.level2;
            return potions.level1;
        }
        return nullptr;
    }

    inline bool HasKnownEffectInIngredient(EffectSetting* effect, IngredientItem* item) {
        auto index = -1;

        for (int i = 0; i < item->effects.size(); i++) {
            auto ingrEffect = item->effects[i];
            if (ingrEffect->baseEffect == effect) {
                index = i;
                break;
            }
        }
        if (index < 0) {
            return false;
        }

        return (item->gamedata.knownEffectFlags & (1 << index)) != 0;
    }

    inline bool HasKnownEffectInIngredients(BGSConstructibleObject* cobj, EffectSetting* effect) {
        if (!constructibleMetadata.contains(cobj)) {
            return false;
        }
        auto metadata = constructibleMetadata[cobj];
        if (!metadata.ingr1 || !metadata.ingr2) {
            return false;
        }
        auto potion = cobj->createdItem->As<AlchemyItem>();
        if (!potion || !potion->effects[0] || !potion->effects[0]->baseEffect) {
            return false;
        }
        auto potionEffect = potion->effects[0]->baseEffect;

        if (!HasKnownEffectInIngredient(potionEffect, metadata.ingr1) ||
            !HasKnownEffectInIngredient(potionEffect, metadata.ingr2)) {
            return false;
        }

        return true;
    }

    inline void CreateRecipes(std::vector<IngredientItem*>& first, std::vector<IngredientItem*>& second,
                              AlchemyItem* potion, int targetLevel, int potionMinLevel) {
        // to avoid creating duplicate recipes when first = second, e.g. common + common
        std::set<unsigned int> createdForFormIds;
        const auto factory = IFormFactory::GetConcreteFormFactoryByType<BGSConstructibleObject>();
        auto playerRef = PlayerCharacter::GetSingleton();
        const auto dataHandler = TESDataHandler::GetSingleton();

        for (int i = 0; i < first.size(); i++) {
            auto ingr1 = first.at(i);

            for (int k = 0; k < second.size(); k++) {
                auto ingr2 = second.at(k);
                if (ingr1 == ingr2) {
                    continue;
                }

                auto ingrFormId = ingr1->GetFormID();
                auto secIngrFormId = ingr2->GetFormID();

                auto key = ingrFormId + secIngrFormId;
                // create recipe if not created already
                if (!createdForFormIds.contains(key)) {
                    createdForFormIds.insert(key);
                    auto obj = factory ? factory->Create() : nullptr;

                    if (obj) {
                        auto baseEffect = potion->effects[0]->baseEffect;
                        auto baseEffectName =
                            baseEffect->GetFullName() ? baseEffect->GetFullName() : baseEffect->GetFormEditorID();

                        log::info("Level {} Recipe: {}, ingr1: {}, ingr2: {}", targetLevel, baseEffectName,
                                  ingr1->GetFullName(), ingr2->GetFullName());
                        obj->benchKeyword = alchemyKeyword;
                        obj->requiredItems.AddObjectToContainer(ingr1, 1, nullptr);
                        obj->requiredItems.AddObjectToContainer(ingr2, 1, nullptr);
                        obj->createdItem = potion;
                        obj->data.numConstructed = 1;

                        auto ingr1Cond = new TESConditionItem;
                        ingr1Cond->next = nullptr;
                        ingr1Cond->data.comparisonValue.f = 1.0f;
                        ingr1Cond->data.functionData.function = FUNCTION_DATA::FunctionID::kGetItemCount;
                        ingr1Cond->data.flags.opCode = CONDITION_ITEM_DATA::OpCode::kGreaterThanOrEqualTo;
                        ingr1Cond->data.functionData.params[0] = ingr1;

                        auto ingr2Cond = new TESConditionItem;
                        ingr2Cond->next = ingr1Cond;
                        ingr2Cond->data.comparisonValue.f = 1.0f;
                        ingr2Cond->data.functionData.function = FUNCTION_DATA::FunctionID::kGetItemCount;
                        ingr2Cond->data.flags.opCode = CONDITION_ITEM_DATA::OpCode::kGreaterThanOrEqualTo;
                        // ingr2Cond->data.flags.isOR = true;
                        ingr2Cond->data.functionData.params[0] = ingr2;

                        auto playerCond = new TESConditionItem;
                        playerCond->next = ingr2Cond;
                        playerCond->data.comparisonValue.f = 0.0f;
                        playerCond->data.functionData.function = FUNCTION_DATA::FunctionID::kGetIsReference;
                        playerCond->data.functionData.params[0] = playerRef;
                        obj->conditions.head = playerCond;

                        constructibleMetadata[obj].potionMinLevel = potionMinLevel;
                        constructibleMetadata[obj].targetIngredientLevel = targetLevel;
                        constructibleMetadata[obj].ingr1 = ingr1;
                        constructibleMetadata[obj].ingr2 = ingr2;

                        dataHandler->GetFormArray<BGSConstructibleObject>().push_back(obj);
                    }
                }
            }
        }
    }

    class EventHandler : public BSTEventSink<TESFurnitureEvent> {
    public:
        static EventHandler* GetSingleton() {
            static EventHandler self;
            return std::addressof(self);
        }
        inline static int count = 1;

        virtual BSEventNotifyControl ProcessEvent(const TESFurnitureEvent* event,
                                                  BSTEventSource<TESFurnitureEvent>* eventSource) override {
            if (!event->actor->IsPlayerRef()) {
                return BSEventNotifyControl::kContinue;
            }

            if (!event->targetFurniture->HasKeyword(alchemyKeyword)) {
                return BSEventNotifyControl::kContinue;
            }

            log::info("Accessing Workbench: {}, {}", event->targetFurniture->GetDisplayFullName(),
                      event->type == TESFurnitureEvent::FurnitureEventType::kEnter ? "Enter" : "Exit");

            auto level1Allowed = true;
            auto level2Allowed = true;
            auto level3Allowed = true;
            auto level4Allowed = false;
            auto level5Allowed = false;

            auto playerCharacter = PlayerCharacter::GetSingleton();

            if (event->type == TESFurnitureEvent::FurnitureEventType::kEnter) {
                for (auto data : constructibleMetadata) {
                    auto cobj = data.first;
                    auto& metadata = data.second;
                    auto potion = cobj->createdItem->As<AlchemyItem>();
                    if (!potion || !potion->effects[0] || !potion->effects[0]->baseEffect) {
                        continue;
                    }

                    int maxAllowedPotionLevel = 1;
                    if (playerCharacter->HasPerk(level2Perk)) {
                        maxAllowedPotionLevel = 2;
                    }
                    if (playerCharacter->HasPerk(level3Perk)) {
                        maxAllowedPotionLevel = 3;
                    }
                    if (playerCharacter->HasPerk(level4Perk)) {
                        maxAllowedPotionLevel = 4;
                    }
                    if (playerCharacter->HasPerk(level5Perk)) {
                        maxAllowedPotionLevel = 5;
                    }

                    // must have perk to access it
                    if (metadata.potionMinLevel == 2 && !playerCharacter->HasPerk(level2Perk)) {
                        continue;
                    }
                    if (metadata.potionMinLevel == 3 && !playerCharacter->HasPerk(level3Perk)) {
                        continue;
                    }
                    if (metadata.potionMinLevel == 4 && !playerCharacter->HasPerk(level4Perk)) {
                        continue;
                    }
                    if (metadata.potionMinLevel == 5 && !playerCharacter->HasPerk(level5Perk)) {
                        continue;
                    }

                    // adjust number of potions constructed if has corresponding perk
                    if (doubleItemsPerk && playerCharacter->HasPerk(doubleItemsPerk)) {
                        cobj->data.numConstructed = 2;
                    }

                    int increaseLevel = 0;

                    if (potionQualityPerk && playerCharacter->HasPerk(potionQualityPerk) && !potion->IsPoison()) {
                        increaseLevel++;
                    }
                    if (poisonQualityPerk && playerCharacter->HasPerk(poisonQualityPerk) && potion->IsPoison()) {
                        increaseLevel++;
                    }
                    if (allQualityPerk && playerCharacter->HasPerk(allQualityPerk)) {
                        increaseLevel++;
                    }

                    if (increaseLevel > 0) {
                        int newLevel = metadata.potionMinLevel + increaseLevel;
                        if (newLevel > maxAllowedPotionLevel) {
                            newLevel = maxAllowedPotionLevel;
                        }
                        auto effect = potion->effects[0]->baseEffect;
                        if (effect && potionsByEffect.contains(effect) && newLevel > metadata.potionMinLevel) {
                            auto potionsVariations = potionsByEffect[effect];
                            auto newPotion = GetHigherLevelPotion(potionsVariations, newLevel);
                            if (newPotion) {
                                cobj->createdItem = newPotion;
                            }
                        }
                    }

                    // Player must know effect in both ingredients
                    auto potionEffect = potion->effects[0]->baseEffect;
                    if (!HasKnownEffectInIngredients(cobj, potionEffect)) {
                        continue;
                    }

                    // unhide recipe
                    if (data.first->conditions.head) {
                        data.first->conditions.head->data.comparisonValue.f = 1.0f;
                    }
                }
            } else {
                // Mark all recipes hidden again
                for (auto data : constructibleMetadata) {
                    if (data.first->conditions.head) {
                        data.first->conditions.head->data.comparisonValue.f = 0.0f;
                    }
                }
            }

            return BSEventNotifyControl::kContinue;
        }
    };
}  // namespace

void AlchmeyDistributor::Initialize() {
    const auto dataHandler = TESDataHandler::GetSingleton();

    alchemyKeyword = TESForm::LookupByID<BGSKeyword>(0x0004F6E6);
    auto craftableKeyword = dataHandler->LookupForm<BGSKeyword>(0x800, "AlchemyReworked.esp");

    level1Keyword = dataHandler->LookupForm<BGSKeyword>(0x801, "AlchemyReworked.esp");
    level2Keyword = dataHandler->LookupForm<BGSKeyword>(0x802, "AlchemyReworked.esp");
    level3Keyword = dataHandler->LookupForm<BGSKeyword>(0x803, "AlchemyReworked.esp");
    level4Keyword = dataHandler->LookupForm<BGSKeyword>(0x804, "AlchemyReworked.esp");
    level5Keyword = dataHandler->LookupForm<BGSKeyword>(0x805, "AlchemyReworked.esp");

    auto commonIngrKeyword = dataHandler->LookupForm<BGSKeyword>(0x806, "AlchemyReworked.esp");
    auto uncommonIngrKeyword = dataHandler->LookupForm<BGSKeyword>(0x807, "AlchemyReworked.esp");
    auto rareIngrKeyword = dataHandler->LookupForm<BGSKeyword>(0x808, "AlchemyReworked.esp");

    auto config = Config::GetSingleton();
    level2Perk = LoadPerkFromConfig(config.GetPerksConfig().level2Perk);
    level3Perk = LoadPerkFromConfig(config.GetPerksConfig().level3Perk);
    level4Perk = LoadPerkFromConfig(config.GetPerksConfig().level4Perk);
    level5Perk = LoadPerkFromConfig(config.GetPerksConfig().level5Perk);
    potionQualityPerk = LoadPerkFromConfig(config.GetPerksConfig().potionQualityPerk);
    poisonQualityPerk = LoadPerkFromConfig(config.GetPerksConfig().poisonQualityPerk);
    allQualityPerk = LoadPerkFromConfig(config.GetPerksConfig().allQualityPerk);
    doubleItemsPerk = LoadPerkFromConfig(config.GetPerksConfig().doubleItemsPerk);

    if (!level2Perk) {
        log::error("Unable to load level2 perk from config");
        return;
    }
    if (!level3Perk) {
        log::error("Unable to load level3 perk from config");
        return;
    }
    if (!level4Perk) {
        log::error("Unable to load level4 perk from config");
        return;
    }
    if (!level5Perk) {
        log::error("Unable to load level5 perk from config");
        return;
    }
    if (config.GetPerksConfig().potionQualityPerk != "") {
        if (potionQualityPerk) {
            log::info("Loaded perk for potion +1 level");
        } else {
            log::info("Unable to load perk for potion +1 level");
        }
    }
    if (config.GetPerksConfig().poisonQualityPerk != "") {
        if (poisonQualityPerk) {
            log::info("Loaded perk for poison +1 level");
        } else {
            log::info("Unable to load perk for poison +1 level");
        }
    }
    if (config.GetPerksConfig().allQualityPerk != "") {
        if (allQualityPerk) {
            log::info("Loaded perk for all +1 level");
        } else {
            log::info("Unable to load perk for all +1 level");
        }
    }
    if (config.GetPerksConfig().doubleItemsPerk != "") {
        if (doubleItemsPerk) {
            log::info("Loaded perk for double potions");
        } else {
            log::info("Unable to load perk for double potions");
        }
    }
    // potion/posion/all quality perks are optional along with doubleItems perk. Someome may want to turn them off

    for (auto& furn : dataHandler->GetFormArray<TESFurniture>()) {
        if (furn && furn->HasKeyword(alchemyKeyword)) {
            log::info("Overrding furniture {}", furn->GetFullName());
            furn->workBenchData.benchType = TESFurniture::WorkBenchData::BenchType::kCreateObject;
        }
    }

    // categorize and store ingredients by rarity
    for (auto ingredientItem : dataHandler->GetFormArray<IngredientItem>()) {
        if (!ingredientItem) {
            continue;
        }

        BGSKeyword* keyword;

        if (ingredientItem->HasKeyword(commonIngrKeyword)) {
            keyword = commonIngrKeyword;
            commonIngredients.push_back(ingredientItem);
            if (config.GetIngrConfig().renameIngredients) {
                ingredientItem->fullName = BSFixedString(std::string(ingredientItem->fullName.c_str()) + " " +
                                                         config.GetIngrConfig().commonSuffix);
            }
        } else if (ingredientItem->HasKeyword(uncommonIngrKeyword)) {
            keyword = uncommonIngrKeyword;
            uncommonIngredients.push_back(ingredientItem);
            if (config.GetIngrConfig().renameIngredients) {
                ingredientItem->fullName = BSFixedString(std::string(ingredientItem->fullName.c_str()) + " " +
                                                         config.GetIngrConfig().uncommonSuffix);
            }
        } else if (ingredientItem->HasKeyword(rareIngrKeyword)) {
            keyword = rareIngrKeyword;
            rareIngredients.push_back(ingredientItem);
            if (config.GetIngrConfig().renameIngredients) {
                ingredientItem->fullName = BSFixedString(std::string(ingredientItem->fullName.c_str()) + " " +
                                                         config.GetIngrConfig().rareSuffix);
            }
        } else {
            // Non-keyworded ingredients are uncommon
            // TODO: make conf option for this
            uncommonIngredients.push_back(ingredientItem);
            keyword = uncommonIngrKeyword;
            if (config.GetIngrConfig().renameIngredients) {
                ingredientItem->fullName = BSFixedString(std::string(ingredientItem->fullName.c_str()) + " " +
                                                         config.GetIngrConfig().uncommonSuffix);
            }
        }

        if (keyword) {
            log::info("Ingredient: {}, {}", ingredientItem->GetFullName(), keyword->GetFormEditorID());
        }
    }

    // Categorize and store potions & poisons by rarity
    for (auto alchItem : dataHandler->GetFormArray<AlchemyItem>()) {
        if (!alchItem) {
            continue;
        }
        if (!alchItem->HasKeyword(craftableKeyword)) {
            continue;
        }
        // skip multi-effect potions
        if (alchItem->effects.size() != 1) {
            continue;
        }
        int level = 0;
        auto potionEffect = alchItem->effects[0]->baseEffect;
        if (alchItem->HasKeyword(level1Keyword)) {
            level = 1;
            potionsByEffect[potionEffect].level1 = alchItem;
        } else if (alchItem->HasKeyword(level2Keyword)) {
            level = 2;
            potionsByEffect[potionEffect].level2 = alchItem;
        } else if (alchItem->HasKeyword(level3Keyword)) {
            level = 3;
            potionsByEffect[potionEffect].level3 = alchItem;
        } else if (alchItem->HasKeyword(level4Keyword)) {
            level = 4;
            potionsByEffect[potionEffect].level4 = alchItem;
        } else if (alchItem->HasKeyword(level5Keyword)) {
            level = 5;
            potionsByEffect[potionEffect].level5 = alchItem;
        }
        if (level != 0) {
            log::info("Processing {}, isPoison: {}, Alchemy Level: {}", alchItem->GetFullName(), alchItem->IsPoison(),
                      level);
        } else {
            log::info("Skipping {}, isPoison: {}, No Alch level assigned", alchItem->GetFullName(),
                      alchItem->IsPoison());
        }
    }

    // create cobj objects
    const auto factory = IFormFactory::GetConcreteFormFactoryByType<BGSConstructibleObject>();
    auto playerRef = PlayerCharacter::GetSingleton();

    for (const auto& data : potionsByEffect) {
        auto effect = data.first;
        auto potions = data.second;

        // int minLevel = 0;
        // auto minPotion = GetEarliestLevelPotion(potions, &minLevel);
        // if (!minPotion || minLevel == 0) {
        //     continue;
        // }

        auto filterEffect = [effect](IngredientItem* ingr) {
            for (const auto ingrEffect : ingr->effects) {
                if (ingrEffect->baseEffect == effect) {
                    return true;
                }
            }
            return false;
        };

        std::vector<IngredientItem*> effectCommonIngredients;
        std::copy_if(commonIngredients.begin(), commonIngredients.end(), std::back_inserter(effectCommonIngredients),
                     filterEffect);

        std::vector<IngredientItem*> effectUncommonIngredients;
        std::copy_if(uncommonIngredients.begin(), uncommonIngredients.end(),
                     std::back_inserter(effectUncommonIngredients), filterEffect);

        std::vector<IngredientItem*> effectRareIngredients;
        std::copy_if(rareIngredients.begin(), rareIngredients.end(), std::back_inserter(effectRareIngredients),
                     filterEffect);

        // ingredient rules:
        // common + common = level 1
        // common + uncommon = level 2
        // common + rare = level 3
        // uncommon + uncommon = level 3
        // uncommon + rare = level 4
        // rare + rare = level 5

        if (potions.level1) {
            CreateRecipes(effectCommonIngredients, effectCommonIngredients, potions.level1, 1, 1);
        }
        if (potions.level2) {
            CreateRecipes(effectCommonIngredients, effectUncommonIngredients, potions.level2, 2, 2);
        }
        if (potions.level3) {
            CreateRecipes(effectCommonIngredients, effectRareIngredients, potions.level3, 3, 3);
            // alt
            CreateRecipes(effectUncommonIngredients, effectUncommonIngredients, potions.level3, 3, 3);
        }
        if (potions.level4) {
            CreateRecipes(effectUncommonIngredients, effectRareIngredients, potions.level4, 4, 4);
        }
        if (potions.level5) {
            CreateRecipes(effectRareIngredients, effectRareIngredients, potions.level5, 5, 5);
        }
    }

    log::info("Total potions: {}", potionsByEffect.size());
    log::info("Total ingredients: {}", commonIngredients.size() + uncommonIngredients.size() + rareIngredients.size());
    log::info("Total recipes: {}", constructibleMetadata.size());

    ScriptEventSourceHolder::GetSingleton()->GetEventSource<TESFurnitureEvent>()->AddEventSink(
        EventHandler::GetSingleton());

    log::info("Initialization Completed");
}
