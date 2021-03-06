#ifndef _CLUSTERTHREAD_
#define _CLUSTERTHREAD_ 1


#include "project.h"

#include <thread>
#include <mutex>

typedef unordered_map < std::string, std::vector<double>> result_map;
typedef std::vector< std::string > keylist;

class ClusterThread
{
    bool
        Finished,
        CancelFlag,
        FinishedWithErrors;

    int _sim_g_start;
    int _sim_g_end;
    int _nsim_complete;
    int _nsim_total;

    std::string _thread_id;
    ssc_data_t _ssc_data;

    Project *_P;
    cluster_sim *_C;
    result_map *_result;
    keylist *_keys;
    std::vector<double> *_sf_avail;
    std::vector<std::string> _sim_messages;
    matrix<double> *_avg_sfavail;

    //wxMutex
    std::mutex
        StatusLock,
        CancelLock,
        FinishedLock,
        FinErrLock;

public:

    void Setup(std::string &tname, Project *P, cluster_sim *C, ssc_data_t ssc_data, std::vector<double> *sf_avail,
        matrix<double> *avg_sfavail, result_map *results, keylist *keys, int sim_g_start, int sim_g_end);

    void CancelSimulation();

    bool IsSimulationCancelled();

    bool IsFinished();

    bool IsFinishedWithErrors();

    void UpdateStatus(int ncomp, int ntot);

    void GetStatus(int *ncomp, int *ntot);

    std::vector<std::string> *GetSimMessages();    //can be called only after simulation is terminated

    void StartThread();

};


#endif 

