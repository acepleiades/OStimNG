#include "Core/Thread.h"

#include "Graph/GraphTable.h"
#include "Util/Constants.h"
#include "Util/VectorUtil.h"

namespace OStim {

#pragma region util
    bool forAnyActor(Graph::Node* node, std::function<bool(Graph::GraphActor&)> condition) {
        for (Graph::GraphActor& actor : node->actors) {
            if (condition(actor)) {
                return true;
            }
        }
        return false;
    }

    bool forAllActors(Graph::Node* node, std::function<bool(Graph::GraphActor&)> condition) {
        for (Graph::GraphActor& actor : node->actors) {
            if (!condition(actor)) {
                return false;
            }
        }
        return true;
    }

    bool forAnyAction(Graph::Node* node, std::function<bool(Graph::Action&)> condition) {
        for (Graph::Action& action : node->actions) {
            if (condition(action)) {
                return true;
            }
        }
        return false;
    }

    void addFurniture(std::vector<std::function<bool(Graph::Node*)>>& conditions, Furniture::FurnitureType* furnitureType) {
        std::string furnitureTypeID = furnitureType->getListType()->id;
        if (furnitureTypeID == "none") {
            conditions.push_back([&](Graph::Node* node) {
                return forAnyActor(node, [&](Graph::GraphActor& actor) {
                    return VectorUtil::contains(actor.tags, std::string("standing"));
                });
            });
        } else if (furnitureTypeID == "bed") {
            conditions.push_back([&](Graph::Node* node) {
                return forAllActors(node, [&](Graph::GraphActor& actor) {
                    return !VectorUtil::contains(actor.tags, std::string("standing"));
                });
            });
        }
    }

    bool checkConditions(std::vector<std::function<bool(Graph::Node*)>>& conditions, Graph::Node* node) {
        for (std::function<bool(Graph::Node*)> condition : conditions) {
            if (!condition(node)) {
                return false;
            }
        }
        return true;
    }

    Graph::Node* getRandomForeplayNode(Graph::Node* currentNode, std::vector<Trait::ActorCondition> actorConditions, Furniture::FurnitureType* furnitureType, bool lesbian, bool gay) {
        std::vector<std::function<bool(Graph::Node*)>> conditions;
        conditions.push_back([&](Graph::Node* node) { return node->findAnyAction({"analsex", "tribbing", "vaginalsex"}) == -1; });
        addFurniture(conditions, furnitureType);
        conditions.push_back([&](Graph::Node* node) {
            return forAnyAction(node, [&](Graph::Action& action) {
                return VectorUtil::contains(action.attributes->tags, std::string("sexual"));
            });
        });
        // TODO do this with intended sex setting instead
        if (lesbian) {
            conditions.push_back([&](Graph::Node* node) { return VectorUtil::contains(node->tags, std::string("lesbian")); });
        } else if (gay) {
            conditions.push_back([&](Graph::Node* node) { return VectorUtil::contains(node->tags, std::string("gay")); });
        }

        if (currentNode->hasActionTag("sexual") && MCM::MCMTable::autoModeLimitToNavigationDistance()) {
            return currentNode->getRandomNodeInRange(MCM::MCMTable::navigationDistanceMax(), actorConditions, [&conditions](Graph::Node* node) { return checkConditions(conditions, node); });
        } else {
            return Graph::GraphTable::getRandomNode(furnitureType, actorConditions, [&conditions](Graph::Node* node) { return checkConditions(conditions, node); });
        }
    }

    Graph::Node* getRandomSexNode(Graph::Node* currentNode, std::vector<Trait::ActorCondition> actorConditions, Furniture::FurnitureType* furnitureType, bool lesbian, bool gay) {
        std::vector<std::function<bool(Graph::Node*)>> conditions;
        conditions.push_back([&lesbian](Graph::Node* node) { return node->findAnyAction(std::vector<std::string>{"analsex", "tribbing", "vaginalsex"}) != -1;
        });
        addFurniture(conditions, furnitureType);
        // TODO do this with intended sex setting instead
        if (lesbian) {
            conditions.push_back([&](Graph::Node* node) { return VectorUtil::contains(node->tags, std::string("lesbian")); });
        } else if (gay) {
            conditions.push_back([&](Graph::Node* node) { return VectorUtil::contains(node->tags, std::string("gay")); });
        }

        if (currentNode->hasActionTag("sexual") && MCM::MCMTable::autoModeLimitToNavigationDistance()) {
            return currentNode->getRandomNodeInRange(MCM::MCMTable::navigationDistanceMax(), actorConditions, [&conditions](Graph::Node* node) { return checkConditions(conditions, node); });
        } else {
            return Graph::GraphTable::getRandomNode(furnitureType, actorConditions, [&conditions](Graph::Node* node) { return checkConditions(conditions, node); });
        }
    }
#pragma endregion

    void Thread::evaluateAutoMode() {
        if (!playerThread) {
            startAutoMode();
            return;
        }

        if (MCM::MCMTable::useAutoModeAlways()) {
            startAutoMode();
            return;
        }

        if (m_actors.size() == 1) {
            if (MCM::MCMTable::useAutoModeSolo()) {
                startAutoMode();
            }
            return;
        }

        // TODO auto mode for dominant or submissive player
        if (MCM::MCMTable::useAutoModeVanilla()) {
            startAutoMode();
        }
    }

    void Thread::startAutoMode() {
        if (autoMode) {
            return;
        }

        if ((threadFlags & ThreadFlag::NO_AUTO_MODE) == ThreadFlag::NO_AUTO_MODE) {
            return;
        }

        if (autoModeStage == AutoModeStage::NONE) {
            if (m_actors.size() == 1) {
                autoModeStage = AutoModeStage::MAIN;
            } else {
                if (RNGUtil::chanceRoll(MCM::MCMTable::autoModeForeplayChance())) {
                    autoModeStage = AutoModeStage::FOREPLAY;
                    foreplayThreshold = RNGUtil::uniformInt(MCM::MCMTable::autoModeForeplayThresholdMin(), MCM::MCMTable::autoModeForeplayThresholdMax());
                } else {
                    autoModeStage = AutoModeStage::MAIN;
                }
                if (RNGUtil::chanceRoll(MCM::MCMTable::autoModePulloutChance())) {
                    pulloutThreshold = RNGUtil::uniformInt(MCM::MCMTable::autoModePulloutThresholdMin(), MCM::MCMTable::autoModePulloutThresholdMax());
                }
            }
        }

        if (m_currentNode && nodeQueue.empty()) {
            if (m_currentNode->hasActionTag("sexual")) {
                startAutoModeCooldown();
            } else {
                progressAutoMode();
            }
        }

        autoMode = true;
    }

    void Thread::startAutoModeCooldown() {
        autoModeCooldown = RNGUtil::uniformInt(MCM::MCMTable::autoModeAnimDurationMin(), MCM::MCMTable::autoModeAnimDurationMax());
    }

    void Thread::progressAutoMode() {
        Graph::Node* next = nullptr;
        if (m_actors.size() == 1) {
            std::string action = GetActor(0)->hasSchlong() ? "malemasturbation" : "femalemasturbation";
            next = Graph::GraphTable::getRandomNode(furnitureType, getActorConditions(), [&action](Graph::Node* node) { return node->findAction(action) != -1; });
        } else {
            // TODO actually do this with intended sex
            bool lesbian = true;
            bool gay = true;
            if (MCM::MCMTable::intendedSexOnly()) {
                for (auto& [index, actor] : m_actors) {
                    if (actor.isFemale()) {
                        gay = false;
                    } else {
                        lesbian = false;
                    }
                }
            } else {
                lesbian = false;
                gay = false;
            }
            if (autoModeStage == AutoModeStage::FOREPLAY) {
                next = getRandomForeplayNode(m_currentNode, getActorConditions(), furnitureType, lesbian, gay);
            } else if (autoModeStage == AutoModeStage::MAIN) {
                next = getRandomSexNode(m_currentNode, getActorConditions(), furnitureType, lesbian, gay);
            }
        }

        if (next) {
            navigateTo(next);
        }
        
        startAutoModeCooldown();
    }

    void Thread::nodeChangedAutoControl() {
        if (autoMode) {
            startAutoModeCooldown();
        }
    }

    void Thread::loopAutoControl() {
        if (!nodeQueue.empty()) {
            return;
        }

        if (!playerThread || MCM::MCMTable::autoSpeedControl()) {
            if ((autoSpeedControlCooldown -= Constants::LOOP_TIME_MILLISECONDS) <= 0) {
                int excitement = getMaxExcitement();
                int chance;
                int min = MCM::MCMTable::autoSpeedControlExcitementMin();
                int max = MCM::MCMTable::autoSpeedControlExcitementMax();
                if (excitement < min) {
                    chance = 0;
                } else if (excitement > max) {
                    chance = 100;
                } else {
                    chance = ((excitement - min) * 100) / (max - min);
                }
                if (RNGUtil::chanceRoll(chance)) {
                    increaseSpeed();
                }
                autoSpeedControlCooldown = RNGUtil::normalInt(MCM::MCMTable::autoSpeedControlIntervalMin(), MCM::MCMTable::autoSpeedControlIntervalMax());
            }
        }


        if (!autoMode) {
            return;
        }

        if (autoModeStage == AutoModeStage::PULLOUT) {
            // wait for climax
            return;
        }

        if (autoModeStage == AutoModeStage::FOREPLAY) {
            if (getMaxExcitement() > foreplayThreshold) {
                autoModeStage = AutoModeStage::MAIN;
                autoModeCooldown = 0;
            }
        }

        if (autoModeStage == AutoModeStage::MAIN && pulloutThreshold != 0) {
            float maxExcitement = 0;
            for (auto& [index, actor] : m_actors) {
                if (actor.hasSchlong() && actor.getExcitement() > maxExcitement) {
                    maxExcitement = actor.getExcitement();
                }
            }

            if (maxExcitement > pulloutThreshold) {
                autoModeStage = AutoModeStage::PULLOUT;
                pullOut();
            }
        }

        if ((autoModeCooldown -= Constants::LOOP_TIME_MILLISECONDS) < 0) {
            progressAutoMode();
        }
    }

    void Thread::setAutoModeToMainStage() {
        if (autoModeStage == AutoModeStage::MAIN) {
            return;
        }

        autoModeStage = AutoModeStage::MAIN;
    }
}