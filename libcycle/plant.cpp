#include "plant.h"
#include "lib_util.h"
#include <algorithm>
#include <iostream>


void CSPPlant::AssignGenerator( WELLFiveTwelve *gen )
{
    m_gen = gen;
}

void CSPPlant::SetSimulationParameters( int read_periods, int num_periods, double epsilon, bool print_output)
{
    m_read_periods = read_periods;
    m_num_periods = num_periods;
    m_output = print_output;
    m_eps = epsilon;
}

void CSPPlant::SetCondenserEfficienciesCold(std::vector<double> eff_cold)
{
	/*
	Mutator for condenser efficiencies when ambient temperature is lower than the
	threshold.
	*/
	int num_streams = 0;
	for (size_t i = 0; i < m_components.size(); i++)
	{
		if (m_components.at(i).GetType() == "CondenserAirstream")
			num_streams++;
	}
	if (eff_cold.size() != num_streams + 1)
		throw std::exception("efficiencies must be equal to one plus number of streams"); 
	m_condenser_efficiencies_cold = eff_cold;
}

void CSPPlant::SetCondenserEfficienciesHot(std::vector<double> eff_hot)
{
	/*
	Mutator for condenser efficiencies when ambient temperature is lower than the
	threshold.
	*/
	int num_streams = 0;
	for (size_t i = 0; i < m_components.size(); i++)
	{
		if (m_components.at(i).GetType() == "CondenserAirstream")
			num_streams++;
	}
	if (eff_hot.size() != num_streams+1)
		throw std::exception("efficiencies must be equal to one plus number of streams");
	m_condenser_efficiencies_hot = eff_hot;
}

void CSPPlant::ReadComponentStatus(
	std::unordered_map< std::string, ComponentStatus > dstat
	)
{
	/*
	Mutator for component status for all components in the plant.
	Used at initialization.
	*/
	m_component_status = dstat;
}

void CSPPlant::SetStatus()
{

    for( std::vector< Component >::iterator it=m_components.begin(); it!= m_components.end(); it++)
    {
        if(m_component_status.find( it->GetName() ) != m_component_status.end() )
        {
            it->ReadStatus( m_component_status.at( it->GetName() ) );
        }
        else
        {
			it->GenerateInitialLifesAndProbs( *m_gen );
        }
    }

	SetPlantAttributes(
		m_maintenance_interval, m_maintenance_duration, 
		m_ramp_threshold, m_downtime_threshold, m_steplength, 
		m_plant_status["hours_to_maintenance"] * 1.0,
		m_plant_status["power_output"] * 1.0, m_plant_status["standby"] && true,
		m_capacity, m_plant_status["time_online"] * 1.0, 
		m_plant_status["time_in_standby"] * 1.0,
		m_plant_status["downtime"] * 1.0,
		m_condenser_temp_threshold
		);

}

std::vector< Component > &CSPPlant::GetComponents()
{
    /*
	accessor for components list.
	*/
    return m_components;
}

std::vector< double > CSPPlant::GetComponentLifetimes()
{

    /*
	retval -- linked list of life remaining of each component.
	*/
	std::vector<double> life;
	std::vector<double> clife;
	for (size_t i = 0; i < m_components.size(); i++)
	{
		clife = m_components.at(i).GetLifetimesAndProbs();
		life.insert(life.end(), clife.begin(), clife.end());
	}
    return life;

}

double CSPPlant::GetHoursToMaintenance()
{
	return m_hours_to_maintenance;
}

std::vector< double >  CSPPlant::GetComponentDowntimes()
{
    /*
	retval -- linked list of downtime remaining of each component.
	*/
    std::vector<double> down;

    for(size_t i=0; i<m_components.size(); i++)
        down.push_back( m_components.at(i).GetDowntimeRemaining() );
    return down;
}

bool CSPPlant::AirstreamOnline()
{
	/*
	Returns true if at least one condenser airstream is operational, and false
	otherwise  Used to determine cycle availability (which is zero if this
	is false, as the plant can't operate with no airstreams.)
	*/
	for (size_t i = 0; i<m_components.size(); i++)
		if (m_components.at(i).GetType() == "CondenserAirstream" && m_components.at(i).IsOperational())
		{
			return true;
		}
	return false;
}

bool CSPPlant::FWHOnline()
{
	/*
	Returns true if at least one feedwater heater is operational, and false
	otherwise.  Used to determine cycle availability (which is zero if this
	is false, as the plant can't operate with no feedwater heaters.)
	*/
	for (size_t i = 0; i<m_components.size(); i++)
		if (m_components.at(i).GetType() == "FeedwaterHeater" && m_components.at(i).IsOperational())
		{
			return true;
		}
	return false;
}

bool CSPPlant::IsOnline()
{

    /*
	retval -- a boolean indicator of whether the plant's power cycle is
    generating power.
	*/
    return m_online; 
}

bool CSPPlant::IsOnStandby()
{
    /*
	retval --  a boolean indicator of whether the plant is in standby 
    mode.
	*/
    return m_standby;
}

double CSPPlant::GetTimeInStandby()
{
	return m_time_in_standby;
}

double CSPPlant::GetTimeOnline()
{
	return m_time_online;
}

double CSPPlant::GetRampThreshold()
{
	return m_ramp_threshold;
}

double CSPPlant::GetSteplength()
{
	return m_steplength;
}

std::unordered_map< std::string, failure_event > CSPPlant::GetFailureEvents()
{
	return m_failure_events;
}

void CSPPlant::AddComponent( std::string name, std::string type, //std::string dist_type, double failure_alpha, double failure_beta, 
		double repair_rate, double repair_cooldown_time,
        double hot_start_penalty, double warm_start_penalty, 
		double cold_start_penalty, double availability_reduction,
		double repair_cost)
{
    m_components.push_back( Component(name, type, //dist_type, failure_alpha, failure_beta, 
		repair_rate, repair_cooldown_time, hot_start_penalty, warm_start_penalty, cold_start_penalty, &m_failure_events, availability_reduction, repair_cost) );
}

void CSPPlant::AddFailureType(std::string component, std::string id, std::string failure_mode,
	std::string dist_type, double alpha, double beta)
{
	for (size_t j = 0; j < m_components.size(); j++)
	{
		if (m_components.at(j).GetName() == component)
		{
			m_components.at(j).AddFailureMode(component,
				id, failure_mode, dist_type, alpha, beta);
			return;
		}

	}
}

void CSPPlant::CreateComponentsFromFile(std::string component_data)
{

    /*
	parses a string containing component information for the plant.
    the components are stored as a linked list, with each component's
    data stored in a dictionary.
    
    dispatch_file -- location of the component input file
    retval - linked list of plant data dictionaries
    
    e.g.:
    name	component_type	              failure_alpha failure_beta	repair_rate	repair_cooldown_time	hot_start_penalty	warm_start_penalty	cold_start_penalty
    >> begin file >> 
    SH1,  heat exchanger (salt-to-steam), 0.001,0.5, 0,0.01,0.02,0.03;
    HP1,  high-pressure turbine,          0.001,0.5,24,0.01,0.02,0.03;
    IP1,  medium-pressure turbine,        0.001,0.5,24,0.01,0.02,0.03;
    LP1,  low-pressure turbine,           0.001,0.5,24,0.01,0.02,0.03;
    ...
	*/

    std::vector< std::string > entries = util::split( component_data, ";" );
    for( size_t i=0; i<entries.size(); i++)
    {
        std::vector< std::string > entry = util::split( entries.at(i), "," );

        if( entry.size() != 8 )
            throw std::exception( "Mal-formed cycle availability model table. Component table must contain 8 entries per row (comma separated values), with each entry separated by a ';'." );

        std::vector< double > dat(6);

        for(size_t j=0; j<6; j++)
            util::to_double( entry.at(j+2), &dat.at(j) );
        //                                  name	  component_type  dist_type failure_alpha failure_beta repair_rate repair_cooldown_time hot_start_penalty warm_start_penalty cold_start_penalty
        m_components.push_back( Component( entry.at(0), entry.at(1), //entry.at(2), dat.at(0),   dat.at(1),  
			dat.at(2),  dat.at(3),           dat.at(4),         dat.at(5),         dat.at(6), &m_failure_events ) );
        
    }
    
}

void CSPPlant::SetPlantAttributes(double maintenance_interval,
				double maintenance_duration,
				double ramp_threshold, double downtime_threshold, 
				double steplength, double hours_to_maintenance,
				double power_output, bool standby, double capacity,
				double temp_threshold, 
				double time_online = 0., double time_in_standby = 0.,
				double downtime = 0.
	)
{
    /*
	Initializes plant attributes.
    retval - none
    
	*/

    m_maintenance_interval = maintenance_interval;
    m_maintenance_duration = maintenance_duration;
    m_ramp_threshold = ramp_threshold;
	m_ramp_threshold_min = 1.1*m_ramp_threshold;
	m_ramp_threshold_max = 2.0*m_ramp_threshold;
    m_downtime_threshold = downtime_threshold;
    m_steplength = steplength;
    m_hours_to_maintenance = hours_to_maintenance;
    m_power_output = power_output;
    m_standby = standby;
    m_online = m_power_output > 0.;
	m_capacity = capacity;
	m_time_in_standby = time_in_standby;
	m_time_online = time_online;
	m_condenser_temp_threshold = temp_threshold;
	m_downtime = downtime;
}

void CSPPlant::SetDispatch(std::unordered_map< std::string, std::vector< double > > &data, bool clear_existing)
{

    /*
    Set's an attribute of the dispatch series. 

    ## availabile attributes include ##
    cost
    standby
    can_cycle_run
    cycle_standby_energy
    thermal_energy
    can_receiver_run
    thermal_storage
    startup_inventory
    receiver_energy
    thermal_energy
    cycle_power
    */

    if( clear_existing )
        m_dispatch.clear();

    for( std::unordered_map< std::string, std::vector< double > >::iterator it=data.begin(); it!= data.end(); it++)
    {
        m_dispatch[ it->first ] = it->second;
    }

    
}

int CSPPlant::NumberOfAirstreamsOnline()
{
	/*
	returns the number of condenser airstreams that are online.
	*/
	int num_streams = 0;
	for (size_t i = 0; i<m_components.size(); i++)
		if (m_components.at(i).GetType() == "CondenserAirstream" && m_components.at(i).IsOperational())
		{
			num_streams++;
		}
	return num_streams;
}

double CSPPlant::GetCondenserEfficiency(double temp)
{
	int num_streams = NumberOfAirstreamsOnline();
	if (temp < m_condenser_temp_threshold)
		return m_condenser_efficiencies_cold[num_streams];
	return m_condenser_efficiencies_hot[num_streams];
}

double CSPPlant::GetCycleAvailability(double temp)
{
	/* 
	Provides the cycles availability, based on components that
	are operational.  Assumes that when multiple components are
	down, the effect on cycle availability is additive

	temp -- ambient temperature (affects condenser efficiency)
	*/
	if (!AirstreamOnline() || !FWHOnline())
		return 0.0;
	double avail = GetCondenserEfficiency(temp);
	for (size_t i = 0; i<m_components.size(); i++)
		if (!m_components.at(i).IsOperational())
			avail -= m_components.at(i).GetAvailabilityReduction();
	if (avail < 0.0)
		return 0.0;
	return avail;
}

void CSPPlant::TestForComponentFailures(double ramp_mult, int t, std::string start, std::string mode)
{
	/*
	Determines whether a component is to fail, based on current dispatch.
	*/
	for (size_t i = 0; i < m_components.size(); i++)
		m_components.at(i).TestForFailure(
			m_steplength, ramp_mult, *m_gen, t - m_read_periods, start, mode
		);
}

bool CSPPlant::AllComponentsOperational()
{

    /*
	retval -- a boolean that is True if no components have experienced
    a failure that has not yet been repaired, and False otherwise.
	*/

    bool ok = true;

    for( size_t i=0; i<m_components.size(); i++)
        ok = ok && m_components.at(i).IsOperational();

    return ok;
    
}

void CSPPlant::PlantMaintenanceShutdown(int t, bool record)
{

    /*
	creates a maintenance event that lasts for a fixed duration.  No
    power cycle operation take place at this time.
    failure_file - output file to record failure
    t -- period index
    record -- True if outputting failure event to file, False o.w.
    
	*/
    for( size_t i=0; i<m_components.size(); i++)
        m_components.at(i).Shutdown( m_maintenance_duration );

	if (record)
	{
		m_failure_events[
					std::to_string(t)+"MAINTENANCE"
				] = failure_event(t, "MAINTENANCE", -1, 0., 0. );
	}
    m_hours_to_maintenance = m_maintenance_interval * 1.0;

}

void CSPPlant::AdvanceDowntime()
{

    /*
	When the plant is not operational, advances time by a period.  This
    updates the repair time and/or maintenance time remaining in the plant.

	*/
	for (size_t i = 0; i < m_components.size(); i++)
	{
		if (!m_components.at(i).IsOperational())
			m_components.at(i).AdvanceDowntime(m_steplength);
	}

}

double CSPPlant::GetRampMult(double power_out)
{

	/*
	Returns the ramping penalty, a multiplier applied to the lifetime
	expended by each component in the plant.  If the change in power output
	compared to the previous time period is larger than the ramp rate threshold
	provided as input, the ramping penalty is returned; otherwise, a
	multiplier of 1 (i.e., no penalty due to ramping) is returned.

	power_out -- power out for current time period
	retval -- floating point multiplier

	*/
	if (power_out <= m_eps)
		return 1.0;
	if (std::fabs(power_out - m_power_output) >= m_ramp_threshold_min)
	{
		double ramp_penalty = m_ramping_penalty_min + (m_ramping_penalty_max - m_ramping_penalty_min)*
			(std::fabs(power_out - m_power_output) - m_ramp_threshold_min ) / (m_ramp_threshold_max - m_ramp_threshold_min);
		ramp_penalty = std::min(ramp_penalty, m_ramping_penalty_max);
		return ramp_penalty;
	}
	return 1.0;

}

void CSPPlant::OperateComponents(double ramp_mult, int t, std::string start, std::string mode)
{

    /*
	Simulates operation by reducing the life of each component according
    to the ramping multiplier given; this also includes any cumulative 
    penalties on the multiplier due to hot, warm or cold starts.
    
    ramp_mult -- ramping penalty (multiplier for life degradation)
    t -- period index (indicator of whether read-only or not)
    mode -- operating mode (e.g., HotStart, Running)
    retval -- None
    
	*/
    //print t - m_read_periods
    bool read_only = (t < m_read_periods);
	for (size_t i = 0; i < m_components.size(); i++) 
	{
		if (m_components.at(i).IsOperational())
			m_components.at(i).Operate(
				m_steplength, ramp_mult, *m_gen, read_only, t - m_read_periods, start, mode
			);
		else
			m_components.at(i).AdvanceDowntime(m_steplength);
	}
}

void CSPPlant::ResetHazardRates()
{

    /*
	resets the plant (restores component lifetimes)
	*/
    for( size_t i=0; i<m_components.size(); i++ )
        m_components.at(i).ResetHazardRate();
        
}

void CSPPlant::StoreComponentState()
{
	/*
	Stores the current state of each component, as of 
	the end of the read-in periods.
	*/
	for (
		std::vector< Component >::iterator it = m_components.begin(); 
		it != m_components.end(); 
		it++
		)
	{
		m_component_status[it->GetName()] = it->GetState();
	}
}

void CSPPlant::StorePlantState()
{
	/*
	Stores the current state of the plant, as of
	the end of the read-in periods.
	*/
	m_plant_status["hours_to_maintenance"] = m_hours_to_maintenance*1.0;
	m_plant_status["power_output"] = m_power_output*1.0;
	m_plant_status["standby"] = m_standby && true;
	m_plant_status["running"] = m_power_output > 1e-8;
	m_plant_status["hours_running"] = m_time_online*1.0;
	m_plant_status["hours_in_standby"] = m_time_in_standby*1.0;
	m_plant_status["downtime"] = m_downtime*1.0;
}

void CSPPlant::StoreState()
{
	/*
	Stores the current state of the plant and each component, as of
	the end of the read-in periods.
	*/
	StoreComponentState();
	StorePlantState();
}

std::string CSPPlant::GetStartMode(int t)
{
	/* 
	returns the start mode as a string, or "none" if there is no start.
	*/
	double power_out = m_dispatch.at("cycle_power").at(t);
	if (power_out > m_eps)
	{
		if (IsOnline())
			return "None";
		if (IsOnStandby())
			return "HotStart";
		else if (m_downtime <= m_downtime_threshold)
			return "WarmStart";
		return "ColdStart";
	}
	return "None";
}

std::string CSPPlant::GetOperatingMode(int t)
{
	/*
	returns the operating mode as a string.
	*/
	double power_out = m_dispatch.at("cycle_power").at(t);
	double standby = m_dispatch.at("standby").at(t);
	if (power_out > m_eps)
	{
		if (IsOnline())
			if (m_time_online <= 1.0 - m_eps)
				return "OF"; //in the first hour of power cycle operation
			else
				return "OO"; //ongoing (>1 hour) power cycle operation
			return "OS";  //starting power cycle operation
	}
	else if (standby >= 0.5)
	{
		if (IsOnStandby())
			if (m_time_in_standby <= 1.0 - m_eps)
				return "SF"; //in first hour of standby
			else
				return "SO"; // ongoing standby (>1 hour)
		return "SS";  // if not currently on standby, then starting standby
	}
	return "OFF";
}

std::vector< double > CSPPlant::RunDispatch()
{

    /*
	runs dispatch for entire time horizon.
    failure_file -- file to output failures
    retval -- array of binary variables that are equal to 1 if the plant 
        is able to operate (i.e., no maintenance or repair events in 
        progress), and 0 otherwise.  This includes the read-in period.
    
	*/
    std::vector< double > operating_periods( m_num_periods, 0 );

    for( int t=0; t<m_num_periods; t++)
    {
		if ( t == m_read_periods )
		{
			StoreState();
			//ajz: I moved the failure events removal to the point at which 
			//we stop reading old failure events and start writing new ones.
			m_failure_events.clear();
		}
		//Shut all components down for maintenance if such an event is 
		//read in inputs, or the hours to maintenance is <= zero.
		if( m_hours_to_maintenance <= 0 && t >= m_read_periods )
        {
            PlantMaintenanceShutdown(t-m_read_periods,true);
        }
		if (
			m_failure_events.find(std::to_string(t) + "MAINTENANCE") 
			!= m_failure_events.end()
			)
		{
			PlantMaintenanceShutdown(t, false);
		}
		//Read in any component failures, if in the read-only stage.
		if (t < m_read_periods)
		{
			for (size_t j = 0; j < m_components.size(); j++)
			{
				for (size_t k = 0; k < m_components.at(j).GetFailureTypes().size(); k++)
				{
					if (m_failure_events.find(std::to_string(t) + m_components.at(j).GetName() + std::to_string(k)) != m_failure_events.end())
					{
						std::string label = std::to_string(t) + m_components.at(j).GetName() + std::to_string(k);
						m_components.at(j).ReadFailure(
							m_failure_events[label].duration,
							m_failure_events[label].new_life,
							m_failure_events[label].fail_idx,
							true //This indicates that we reset all hazard rates after repair; may want to revisit
						);

						if (m_output)
							output_log.push_back(util::format("Failure Read: %d, %d, %s", t, m_read_periods, m_failure_events[label].print()));
					}
				}
			}
		}
		// If cycle availability is zero or there is no power output, advance downtime.  
		// Otherwise, check for failures, and operate if there is still availability.
		double power_output = m_dispatch.at("cycle_power").at(t);
		double cycle_avail = GetCycleAvailability(m_dispatch.at("ambient_temperature").at(t));
		std::string start = GetStartMode(t);
		std::string mode = GetOperatingMode(t);
		if (cycle_avail < m_eps)
		{
			power_output = 0.0;
			mode = "OFF";
		}
		if (t >= m_read_periods)  //we only generate 'new' failures after read-in period
		{
			double ramp_mult = GetRampMult(power_output);
			TestForComponentFailures(ramp_mult, t, start, mode);
			cycle_avail = GetCycleAvailability(m_dispatch.at("ambient_temperature").at(t));
			if (cycle_avail < m_eps)
			{
				power_output = 0.0;
				mode = "OFF";
			}
		}
		power_output = std::min(power_output, cycle_avail*m_capacity);
		Operate(power_output, t, start, mode);
		operating_periods[t] = cycle_avail;

		//std::cerr << "Period " << std::to_string(t) << " availability: " << std::to_string(cycle_avail) << ".  Mode: " << mode <<"\n";
    }

	return operating_periods;                   
}

void CSPPlant::Operate(double power_out, int t, std::string start, std::string mode)
{

    /*
	
    Simulates operation by:
        (i) advancing downtime one period if any components are under 
        repair or maintenance;
        (ii) producing a component failure event, followed by (i), and
        recording the event, if the period is not read-only and
        dispatch provided as input would reduce a component's life to zero;
        (iii) operating the plant and updating component lifetimes 
        otherwise.
		power_out -- power output for plant (determines ramping multiple)
		t -- time index
		mode -- operation mode
	*/
	double ramp_mult = GetRampMult(power_out);
	m_power_output = power_out;
	if (mode == "OFF")
	{
		m_online = false;
		m_standby = false;
		m_downtime += m_steplength;
		m_time_in_standby = 0.0;
		m_time_online = 0.0;
		AdvanceDowntime();
		return;
	}
	else if (mode == "SS") //standby - start
	{
		m_online = false;
		m_standby = true;
		m_time_in_standby = m_steplength;
		m_downtime = 0.0;
		m_time_online = 0.0;
	}
	else if (mode == "SF" || mode == "SO") //standby - first hour; standby ongoing (>1 hour)
	{
		m_time_in_standby += m_steplength;
	}
	else if (mode == "OS") //online - start
	{
		m_online = true;
		m_standby = false;
		m_time_in_standby = 0.0;
		m_downtime = 0.0;
		m_time_online = m_steplength;
		m_hours_to_maintenance -= m_steplength;
	}
	else if (mode == "OF" || mode == "OO") //standby - first hour; standby ongoing (>1 hour)
	{
		m_time_online += m_steplength;
		m_hours_to_maintenance -= m_steplength;
	}
	else
		throw std::exception("invalid operating mode.");
	OperateComponents(ramp_mult, t, start, mode); 
}

std::vector< double > CSPPlant::Simulate(bool reset_status)
{

    /*failure_file = open(
        os.path.join(m_output_dir,"component_failures.csv"),'w'
        )*/
    std::vector< double > operating_periods = RunDispatch();
	if (reset_status)
	{
		SetStatus();
	}
	return operating_periods;
    //outfile = open(
    //    os.path.join(m_output_dir,"operating_periods.txt"),'w'
    //    )
    //for i in range(len(operating_periods)):
    //    outfile.write(str(operating_periods[i])+",")
    //failure_file.close()
    //outfile.close()
}