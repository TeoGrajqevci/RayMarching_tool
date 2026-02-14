#pragma once

#include <string>
#include <vector>

#include "rmt/app/benchmark/BenchmarkRunner.h"

namespace rmt {

bool writeBenchmarkCsvReport(const std::string& csvOutputPath,
                             const std::vector<BenchmarkCaseResult>& benchmarkResults);

} // namespace rmt
