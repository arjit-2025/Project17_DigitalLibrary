// scenarios.h - The three mandatory demonstration scenarios.
//   1. Normal operation        2. Capacity / constraint shock        3. Large-data stress
// Each scenario compares at least two valid strategies on the same input.
#ifndef SCENARIOS_H
#define SCENARIOS_H

#include <iosfwd>
#include <string>

void runScenarioNormal(std::ostream& out);
void runScenarioShock(std::ostream& out);
void runScenarioStress(std::ostream& out, const std::string& csvPath);

#endif
