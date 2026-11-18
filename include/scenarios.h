/* scenarios.h - The three mandatory demonstration scenarios.
 *   1. Normal operation   2. Capacity / constraint shock   3. Large-data stress
 * Each compares at least two valid strategies on the same input (fixed seeds). */
#ifndef SCENARIOS_H
#define SCENARIOS_H

void run_scenario_normal(void);
void run_scenario_shock(void);
void run_scenario_stress(const char *csv_path);   /* csv_path may be NULL */

#endif
