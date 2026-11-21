#pragma once

#include <cstdint>

namespace gamedev
{
//
// Simple statistics system (course framework): per-frame values with a history, shown as graphs in the debug UI.

/// Opaque state of the statistics system; created by `initializeStatistics`.
struct StatisticsState;

/// How a statistic is displayed.
enum class StatType
{
    NONE = 0, ///< Not set.
    counter,  ///< Count per frame; graph starts at zero.
    timing    ///< Time per frame in milliseconds.
};

/// Allocates room for `maxNumStats` statistics with a history of `numFrames` frames each. Free with `destroyStatistics`.
StatisticsState* initializeStatistics(uint32_t maxNumStats, uint32_t numFrames);

/// Frees `state`, which must not be used afterwards.
void destroyStatistics(StatisticsState* state);

/// Ends the current frame: appends every stat's frame value to its history and resets it to zero.
void newStatFrame(StatisticsState* state);

/**
 * Returns the frame value of the stat `identifier` (created on first use); add to it during the frame.
 * `identifier` must outlive `state`. Asserts if the stat exists with another type or if all slots are used.
 */
float* getStat(StatisticsState* state, char const* identifier, StatType type);

/// Draws the "Statistics" window with a graph per stat.
void runStatImgui(StatisticsState* state);


/**
 * Plots `numValues` values with min/max/average/current labels; a more detailed variant of `ImGui::PlotLines`.
 * `mainText` is drawn inside the plot, `label` is the ImGui ID, `unit` is appended to the numbers.
 * The value range is rounded to multiples of 5 and starts at 0 if `startAtZero`.
 */
void ImGuiValueGraph(float const* values, uint32_t numValues, char const* mainText, char const* label, char const* unit, float sizeX, float sizeY, bool startAtZero = false);
}
