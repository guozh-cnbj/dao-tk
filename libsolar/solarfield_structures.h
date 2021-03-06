#ifndef _SF_STRUCTURES_ 
#define _SF_STRUCTURES_

#include <vector>
#include <string>

#include "solarfield_heliostat.h"
#include "solarfield_staff.h"
#include "../libclearsky/clearsky.h"

struct solarfield_settings
{
	//-- Simulation duration and resolution
	int n_years;	// Number of years to simulate
	double step;	// Timestep [hr]


	//-- Staff 
	int n_om_staff;					// Total number of full-time staff
	double max_hours_per_day;		// Maximum time per day for full-time staff [hr]
	double max_hours_per_week;		// Maximum time per week for full-time staff [hr]

	//-- Heliostats
	int n_helio;
	int n_helio_sim;
	std::vector<helio_component_inputs> helio_components;  // Heliostat component inputs
	std::vector<double> helio_performance;				   // Heliostat annual energy output [kWh]
	double sum_performance;      //[kWh]

	//-- Heliostat operating hours
	bool is_fix_hours;			// Fix operating hours?
	double sunrise;				// Time [hr] at sunrise (applied to all days, only used if m_is_fixed_hours = true)
	double sunset;				// Time [hr] at sunset (applied to all days, only used if m_is_fixed_hours = true)
	s_location location;		// Location (only used if m_is_fixed_hours = false)
	std::vector<double> op_schedule;  //calculated from clearsky as needed


	//-- Simulation options
	int seed;					// Random number seed
	unsigned int repair_order;	// Order that repairs are prioritized: FAILURE_ORDER, PERFORMANCE, REPAIR_TIME, MEAN_REPAIR_TIME, RANDOM
	bool is_tracking;			// Enable detailed tracking of outputs?

	//bool is_allow_multiple_per_helio;		// Allow multiple staff to work on repairs of a given heliostat in any timestep
	bool is_trade_repairs;					// One one staff member at a time may work on a given heliostat, but allow staff member to take over partially repaired heliostat if original staff member is limited by max hours per day or per week
	//bool is_prioritize_partial_repairs;		// Always prioritize partially-repaired heliostats?
	//bool is_split_operation;				// Allow heliostats to be put back in operation during timestep repairs are completed

	solarfield_settings();

}; 


struct solarfield_results
{
	double avg_avail;			// Overall average availability 
	double min_avail;			// Minimum availability during the simulated period

	std::vector<double> avail_schedule;		// Availability schedule
	std::vector<double> yearly_avg_avail;	// Yearly-average availability
	

	double n_repairs;								// Total number of repairs completed
	std::vector<double> n_repairs_per_component;	// Total number of repairs per component
	std::vector<double> n_failures_per_component;	// Total number of failures per component
	std::vector<double> repair_cost_per_year;         // Total repair cost by year

	double staff_utilization;						
	
	double heliostat_repair_cost_y1;
	double heliostat_repair_cost;

	// Optional outputs for debugging if "is_tracking" = true
	std::vector<double> staff_time;							// Total time worked per staff member[hr]

	std::vector<std::vector<double>> failures_per_year;		// Number of failures per component per year
	std::vector<std::vector<double>> repair_time_per_year;	// Repair time per component per year

	std::vector<double> queue_size_vs_time;			// Fraction of heliostats in queue vs time

	void print();
	void print_avail_and_queue_schedule();

};

struct solarfield_event
{

	int helio_id;
	int component_idx;
	bool is_repair;
	double time;
	double priority;

	solarfield_event() {};

	solarfield_event(
		int _helio_id,
		int _component_idx,
		bool _is_repair,
		double _time,
		double _priority
	) : helio_id(_helio_id),
		component_idx(_component_idx),
		is_repair(_is_repair),
		time(_time),
		priority(_priority)
	{};

	void print_line();
	void print();


};

bool operator<(const solarfield_event& e1, const solarfield_event& e2);

struct solarfield_opt_settings
{
	//inputs
	int max_num_staff;
	double hourly_cost_per_staff;  //per hour
	double labor_discount_rate;
	double revenue_discount_rate;
	double repair_discount_rate;
	double temporary_staff_cost_multiple; //in m^2 per hour
	double system_efficiency; //assumed efficiency including receiver, TES losses, power cycle
	double price_per_kwh;  //assumed average for grid output
	double num_years;      //years of operation to calculate NPV of annual costs
	
	solarfield_opt_settings() {};
	//void print();
};

#endif