#include "BaseConditions.h"

size_t ICondition::EvaluationKeyHash::operator()(const EvaluationKey& a_key) const
{
	auto hash = std::hash<uint32_t>{}(a_key.actorFormID);
	hash ^= std::hash<const void*>{}(a_key.clipGenerator) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	return hash;
}

void ICondition::RecordLastEvalResult(
	RE::TESObjectREFR* a_refr, RE::hkbClipGenerator* a_clipGen, bool a_result) const
{
	const EvaluationKey key{
		a_refr ? a_refr->GetFormID() : 0,
		a_clipGen
	};
	std::unique_lock lock(evalResultsMutex);
	if (!evalResults.contains(key) && evalResults.size() >= 64) {
		auto oldest = std::ranges::min_element(evalResults, [](const auto& a_lhs, const auto& a_rhs) {
			return a_lhs.second.sequence < a_rhs.second.sequence;
		});
		evalResults.erase(oldest);
	}
	evalResults[key] = EvaluationResult{ a_result, ++nextEvalSequence };
}

std::optional<bool> ICondition::GetLastEvalResult(
	RE::TESObjectREFR* a_refr, RE::hkbClipGenerator* a_clipGen) const
{
	const EvaluationKey key{
		a_refr ? a_refr->GetFormID() : 0,
		a_clipGen
	};
	std::shared_lock lock(evalResultsMutex);
	const auto it = evalResults.find(key);
	if (it == evalResults.end()) return std::nullopt;
	return it->second.value;
}

std::optional<bool> ICondition::GetLastPlayerEvalResult() const
{
	const auto* player = RE::PlayerCharacter::GetSingleton();
	if (!player) return std::nullopt;
	const auto playerFormID = player->GetFormID();

	std::shared_lock lock(evalResultsMutex);
	const EvaluationResult* latest = nullptr;
	for (const auto& [key, result] : evalResults) {
		if (key.actorFormID == playerFormID && (!latest || result.sequence > latest->sequence)) {
			latest = &result;
		}
	}
	if (!latest) return std::nullopt;
	return latest->value;
}
