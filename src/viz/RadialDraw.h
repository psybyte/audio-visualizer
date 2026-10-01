#pragma once

#include "analysis/Analyzer.h"
#include "Types.h"

class Batch;

void drawRadial(Batch& batch, const AnalysisSnapshot& audio, Palette palette, float time, int width, int height);
