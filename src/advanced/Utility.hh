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

/**
 * Creates an empty statistics state. Free it with `destroyStatistics`.
 *
 * @param maxNumStats Number of stats that can be created with `getStat`.
 * @param numFrames History length per stat, in frames.
 * @return The new state (owned by the caller).
 */
StatisticsState* initializeStatistics(uint32_t maxNumStats, uint32_t numFrames);

/**
 * Frees a statistics state and all its stats.
 *
 * @param state State from `initializeStatistics`; it and all pointers from `getStat` are invalid afterwards.
 */
void destroyStatistics(StatisticsState* state);

/**
 * Ends the current frame: appends every stat's frame value to its history and resets it to zero.
 *
 * `Game::onFrameStart` calls this once per frame.
 *
 * @param state Statistics to advance.
 */
void newStatFrame(StatisticsState* state);

/**
 * Returns the frame value of a stat, creating the stat on first use; add to it during the frame.
 *
 * Asserts if the stat exists with another type or if all slots are used.
 *
 * @param state Statistics to look the stat up in.
 * @param identifier Name of the stat, also shown in the UI. Not copied, so it must outlive `state`.
 * @param type Display type; must match the type of an existing stat.
 * @return Frame value of the stat, valid until `destroyStatistics`.
 */
float* getStat(StatisticsState* state, char const* identifier, StatType type);

/**
 * Draws the "Statistics" window with a graph per stat, grouped into timings and counters.
 *
 * @param state Statistics to show.
 */
void runStatImgui(StatisticsState* state);


/**
 * Plots values with min/max/average/current labels; a more detailed variant of `ImGui::PlotLines`.
 *
 * The labels are drawn to the right of the plot.
 * The value range is rounded to multiples of 5, is at least 10 wide, and starts at 0 if `startAtZero`.
 *
 * @param values Values to plot, oldest first.
 * @param numValues Number of `values`; must be at least 1.
 * @param mainText Text drawn inside the plot.
 * @param label ImGui label (ID) of the plot.
 * @param unit Suffix of all numbers, e.g. "ms".
 * @param sizeX Plot width in pixels.
 * @param sizeY Plot height in pixels.
 * @param startAtZero Start the value range at 0 instead of near the minimum.
 */
void ImGuiValueGraph(float const* values, uint32_t numValues, char const* mainText, char const* label, char const* unit, float sizeX, float sizeY, bool startAtZero = false);
}
