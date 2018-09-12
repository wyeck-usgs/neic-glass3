/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef GLASS_H
#define GLASS_H

#include <json.h>
#include <string>
#include <memory>
#include <atomic>
#include "TTT.h"
#include "TravelTime.h"

namespace glasscore {

// forward declarations
class CWebList;
class CSiteList;
class CPickList;
class CHypoList;
class CDetection;
class CCorrelationList;
struct IGlassSend;

/**
 * \brief glasscore interface class
 *
 * The CGlass class is the class that sets up and maintains the glass
 * association engine, and acts as the interface between the glasscore library
 * and any clients.
 *
 * CGlass initializes the traveltime library, allocates the site, pick, and
 * hypo lists, creates and maintains the detection web, and manages
 * communication between glasscore and clients via the dispatch function
 * (receiving) and an IGlassSend interface pointer variable (sending).
 *
 * CGlass also performs traveltime library testing during initialization,
 * time encoding/decoding as well as calculating the significance functions
 * and normal distributions as needed.
 *
 * All communication (configuration, input data, or output results ) to / from
 * CGlass is via deserialized json messages as pointers to supereasyjson
 * json::objects.
 */
class CGlass {
 public:
	/**
	 * \brief CGlass constructor
	 *
	 * The constructor for the CGlass class.
	 * Sets allocated lists and objects to null.
	 * Initializes members to default values.
	 */
	CGlass();

	/**
	 * \brief CGlass destructor
	 *
	 * The destructor for the CGlass class.
	 * Cleans up all memory allocated to lists and objects.
	 */
	~CGlass();

	/**
	 * \brief CGlass clear function
	 *
	 */
	static void clear();

	/**
	 * \brief CGlass communication receiving function
	 *
	 * The function used by CGlass to receive communication
	 * (such as configuration or input data), from outside the
	 * glasscore library.
	 *
	 * CGlass will forward the communication on to the pick, site,
	 * or hypo lists, or the detection web if CGlass cannot
	 * use the communication.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CGlass,
	 * false otherwise
	 */
	bool dispatch(std::shared_ptr<json::Object> com);

	void setSend(glasscore::IGlassSend *newSend);

	/**
	 * \brief CGlass communication sending function
	 *
	 * The function used by CGlass to send communication
	 * (such as output data), to outside the glasscore library
	 * using an IGlassSend interface pointer.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was sent via
	 * a valid IGlassSend interface pointer, false otherwise
	 */
	static bool send(std::shared_ptr<json::Object> com);

	/**
	 * \brief CGlass initialization function
	 *
	 * The function used by CGlass to initialize the glasscore
	 * library.  This function loads the earth model, sets the
	 * association parameters, sets up  the ray path calculator,
	 * creates the detection web configures and tests the phase
	 * and branch travel times used for association, creates the
	 * pick, site, and hypo lists, and sets up the output format
	 *
	 * \param com - A pointer to a json::object containing the
	 * configuration to use in initialization.
	 * \return Returns true if the initialization was successful,
	 * false otherwise
	 */
	static bool initialize(std::shared_ptr<json::Object> com);

	/**
	 * \brief check to see if each thread is still functional
	 *
	 * Checks each thread to see if it is still responsive.
	 */
	static bool healthCheck();

	/**
	 * \brief Gets the azimuth window used to determine whether a beam can be
	 * matched with an existing hypo
	 * \return Returns a double containing the beam matching azimuth window in
	 * degrees
	 */
	static double getBeamMatchingAzimuthWindow();

	/**
	 * \brief Gets the distance window used to determine whether a beam can be
	 *  matched with an existing hypo
	 * \return Returns a double containing the beam matching distance window in
	 * degrees
	 */
	static double getBeamMatchingDistanceWindow();

	/**
	 * \brief Gets the correlation cancel age used to determine when a hypo
	 * created by a correlation can be canceled if no other supporting data
	 * exists
	 * \return Returns an integer containing the correlation cancel age in
	 * seconds
	 */
	static int getCorrelationCancelAge();

	/**
	 * \brief Gets the time window used to determine whether a correlation can
	 * be matched with an existing hypo
	 * \return Returns a double containing the correlation matching time window
	 * in seconds
	 */
	static double getCorrelationMatchingTimeWindow();

	/**
	 * \brief Gets the distance window used to determine whether a correlation
	 * can be matched with an existing hypo
	 * \return Returns a double containing the correlation matching distance
	 * window in degrees
	 */
	static double getCorrelationMatchingDistanceWindow();

	/**
	 * \brief Gets the time window used to determine whether a hypo could
	 * be merged with an existing hypo
	 * \return Returns a double containing the hypo merging time window  in
	 * seconds
	 */
	static double getHypoMergingTimeWindow();

	/**
	 * \brief Gets the distance window used to determine whether a hypo
	 * could be merged with an existing hypo
	 * \return Returns a double containing the hypo merging distance window in
	 * degrees
	 */
	static double getHypoMergingDistanceWindow();

	/**
	 * \brief Gets the factor used in calculating the association distance
	 * cutoff factor for pick data
	 * \return Returns a double containing the distance cutoff factor
	 */
	static double getDistanceCutoffFactor();

	/**
	 * \brief Gets the minimum allowed association distance cutoff
	 * \return Returns a double containing the minimum allowed association
	 * distance cutoff in degrees
	 */
	static double getMinDistanceCutoff();

	/**
	 * \brief Gets the percentage used in calculating the association distance
	 * cutoff factor for pick data
	 * \return Returns a double containing the distance cutoff percentage
	 */
	static double getDistanceCutoffPercentage();

	/**
	 * \brief Gets the minimum bayesian stack threshold needed to report a hypo
	 * out of glass
	 * \return Returns a double containing the minimum reporting stack threshold
	 */
	static double getReportingStackThreshold();

	/**
	 * \brief Gets the minimum bayesian stack threshold required to nucleate
	 * an event
	 * \return Returns a double containing the minimum nucleation stack threshold
	 */
	static double getNucleationStackThreshold();

	/**
	 * \brief Gets the exponential pick affinity factor used in association
	 * \return Returns a double containing the exponential factor used for pick
	 * affinity
	 */
	static double getPickAffinityExpFactor();

	/**
	 * \brief Gets a flag indicating whether to produce graphics files for
	 * location tuning
	 * \return Returns a boolean flag indicating whether to output graphics
	 * files for location tuning
	 */
	static bool getGraphicsOut();

	/**
	 * \brief Gets a string indicating the desired graphics output folder
	 * \return Returns a std::string containing the folder to output graphics
	 * files to
	 */
	static const std::string& getGraphicsOutFolder();

	/**
	 * \brief Get the size of the step used in generating graphic output files
	 * \return Returns a double containing the graphics step size in kilometers
	 */
	static double getGraphicsStepKm();

	/**
	 * \brief Gets the number steps used in generating graphic output files
	 * \return Returns an integer containing the number of graphic steps
	 */
	static int getGraphicsSteps();

	/**
	 * \brief Gets the maximum number of times a hypocenter can reprocess without
	 * new data being associated to it
	 * \return Returns an integer containing the limit of hypo processing cycles
	 */
	static int getProcessLimit();

	/**
	 * \brief Gets a flag indicating whether to use the minimizing travel time
	 * locator
	 * \return Returns a boolean flag indicating whether to use the minimizing
	 * travel time locator
	 */
	static bool getMinimizeTTLocator();

	/**
	 * \brief Get the maximum number of sites link to a node
	 * \return Returns an integer containing the maximum number of sites link to
	 * a node
	 */
	static int getNumStationsPerNode();

	/**
	 * \brief Gets the minimum number of data required to nucleate an event
	 * \return Returns an integer containing the minimum number of data required
	 * to nucleate an event
	 */
	static int getNucleationDataThreshold();

	/**
	 * \brief Gets the minimum number of data required to report an event
	 * \return Returns an integer containing the minimum number of data required
	 * to report an event
	 */
	static double getReportingDataThreshold();

	/**
	 * \brief Gets the time window used when checking whether an input pick
	 * is a duplicate of a pick already in the pick list
	 * \return Returns a double containing the pick duplication time window in
	 * seconds
	 */
	static double getPickDuplicateTimeWindow();

	/**
	 * \brief Gets the standard deviation cutoff used for association
	 * \return Returns a double containing the standard deviation cutoff used
	 * for association
	 */
	static double getAssociationSDCutoff();

	/**
	 * \brief Gets the standard deviation cutoff used for pruning
	 * \return Returns a double containing the standard deviation cutoff used
	 * for pruning
	 */
	static double getPruningSDCutoff();

	/**
	 * \brief Gets a flag indicating whether to output locator testing files
	 * \return Returns a boolean flag indicating whether to output locator
	 * testing files
	 */
	static bool getTestLocator();

	/**
	 * \brief Gets a flag indicating whether to test travel times
	 * \return Returns a boolean flag indicating whether to test travel times
	 */
	static bool getTestTravelTimes();

	/**
	 * \brief Get the maximum number of picks to store in glass
	 * \return Returns an integer containing the maximum number of picks
	 */
	static int getMaxNumPicks();

	/**
	 * \brief Set the maximum number of picks to store in glass
	 * \param max - an integer containing the maximum number of picks
	 */
	static void setMaxNumPicks(int max);

	/**
	 * \brief Get the maximum number of correlations to store in glass
	 * \return Returns an integer containing the maximum number of correlations
	 */
	static int getMaxNumCorrelations();

	/**
	 * \brief Set the maximum number of correlations to store in glass
	 * \param max - an integer containing the maximum number of correlations
	 */
	static void setMaxNumCorrelations(int max);

	/**
	 * \brief Get the maximum number of picks to store with a site
	 * \return Returns an integer containing the maximum number of picks to
	 * store with a site
	 */
	static int getMaxNumPicksPerSite();

	/**
	 * \brief Set the maximum number of picks to store with a site
	 * \param max - an integer containing the maximum number of picks to
	 * store with a site
	 */
	static void setMaxNumPicksPerSite(int max);

	/**
	 * \brief Get the maximum number of hypos to store in glass
	 * \return Returns an integer containing the maximum number of hypos
	 */
	static int getMaxNumHypos();

	/**
	 * \brief Set the maximum number of hypos to store in glass
	 * \param max - an integer containing the maximum number of hypos
	 */
	static void setMaxNumHypos(int max);

	/**
	 * \brief Gets a pointer to the Correlation list
	 * \return Returns a pointer to the correlation list
	 */
	static CCorrelationList* getCorrelationList();

	/**
	 * \brief Gets a pointer to the detection processor
	 * \return Returns a pointer to the detection processor
	 */
	static CDetection* getDetectionProcessor();

	/**
	 * \brief Gets a pointer to the Hypo list
	 * \return Returns a pointer to the hypo list
	 */
	static CHypoList* getHypoList();

	/**
	 * \brief Gets a pointer to the Pick list
	 * \return Returns a pointer to the pick list
	 */
	static CPickList* getPickList();

	/**
	 * \brief Gets a pointer to the Site list
	 * \return Returns a pointer to the site list
	 */
	static CSiteList* getSiteList();

	/**
	 * \brief Gets a pointer to the Web list
	 * \return Returns a pointer to the web list
	 */
	static CWebList* getWebList();

	/**
	 * \brief Gets the default nucleation travel time
	 * \return Returns a shared_ptr to the CTravelTime containing the default
	 *  nucleation travel time
	 */
	static std::shared_ptr<traveltime::CTravelTime>& getDefaultNucleationTravelTime(); // NOLINT

	/**
	 * \brief Gets the list of association travel times
	 * \return Returns a shared_ptr to the CTTT containing the list of
	 * association travel times
	 */
	static std::shared_ptr<traveltime::CTTT>& getAssociationTravelTimes();

 private:
	/**
	 * \brief A double value containing the default number of data that
	 * that need to be gathered to trigger the nucleation of an event.
	 * This value can be overridden in a detection grid (Web) if provided as
	 * part of a specific grid setup.
	 */
	static int m_iNucleationDataThreshold;

	/**
	 * \brief A double value containing the default viability threshold needed
	 * to exceed for a nucleation to be successful.
	 * This value can be overridden in a detection grid (Web) if provided as
	 * part of a specific grid setup.
	 */
	static double m_dNucleationStackThreshold;

	/**
	 * \brief A double value containing the default number of closest stations
	 * to use  when generating a node for a detection array.
	 * This value can be overridden in a detection grid (Web) if provided as
	 * part of a specific grid setup.
	 */
	static int m_iNumStationsPerNode;

	/**
	 * \brief A double value containing the standard deviation cutoff used for
	 * associating a pick with a hypocenter.
	 */
	static double m_dAssociationSDCutoff;

	/**
	 * \brief A double value containing the standard deviation cutoff used for
	 * pruning a pick from a hypocenter.
	 */
	static double m_dPruningSDCutoff;

	/**
	 * \brief A double value containing the exponential factor used when
	 * calculating the affinity of a pick with a hypocenter.
	 */
	static double m_dPickAffinityExpFactor;

	/**
	 * \brief A double value containing the factor used to calculate a hypo's
	 * distance cutoff
	 */
	static double m_dDistanceCutoffFactor;

	/**
	 * \brief A double value containing the percentage used to calculate a
	 *  hypo's distance cutoff
	 */
	static double m_dDistanceCutoffPercentage;

	/**
	 * \brief A double value containing the minimum distance cutoff in degrees
	 */
	static double m_dMinDistanceCutoff;

	/**
	 * \brief An integer containing the maximum number of picks stored by
	 * pPickList
	 */
	static int m_iMaxNumPicks;

	/**
	 * \brief An integer containing the maximum number of correlations stored by
	 * pCorrelationList
	 */
	static int m_iMaxNumCorrelations;

	/**
	 * \brief An integer containing the maximum number of picks stored by
	 * the vector in a site
	 */
	static int m_iMaxNumPicksPerSite;

	/**
	 * \brief An integer containing the maximum number of hypocenters stored by
	 * pHypoList
	 */
	static int m_iMaxNumHypos;

	/**
	 * \brief Window in seconds to check for 'duplicate' picks at same station.
	 * If new pick is within window, it isn't added to pick list.
	 */
	static double m_dPickDuplicateTimeWindow;

	/**
	 * \brief Time Window to check for matching correlations in seconds. Used
	 * for checking for duplicate correlations and associating correlations to
	 * hypos
	 */
	static double m_dCorrelationMatchingTimeWindow;

	/**
	 * \brief Distance Window to check for matching correlations in degrees.
	 * Used for checking for duplicate correlations and associating correlations
	 * to hypos
	 */
	static double m_dCorrelationMatchingDistanceWindow;

	/**
	 * \brief Time Window to check for merging hypos in seconds. Used
	 * for checking when hypos can be merged together
	 */
	static double m_dHypoMergingTimeWindow;

	/**
	 * \brief Distance Window to check for merging hypos in degrees.
	 * Used for checking when hypos can be merged together
	 */
	static double m_dHypoMergingDistanceWindow;

	/**
	 * \brief age of correlations before allowing cancel in seconds
	 */
	static int m_iCorrelationCancelAge;

	/**
	 * \brief Azimuth Window to check for matching beams in degrees. Used for
	 * nucleating beams and associating beams to hypos
	 */
	static double m_dBeamMatchingAzimuthWindow;

	/**
	 * \brief Distance Window to check for matching beams in degrees. Used for
	 * nucleating beams and associating beams to hypos
	 */
	static double m_dBeamMatchingDistanceWindow;

	/**
	 * \brief Bool to decide when to print out travel-times.
	 */
	static bool m_bTestTravelTimes;

	/**
	 * \brief Bool to decide when to print files for locator test
	 */
	static bool m_bTestLocator;

	/**
	 * \brief Flag indicating whether to output info for graphics.
	 */
	static bool m_bGraphicsOut;

	/**
	 * \brief Output locations info for graphics.
	 */
	static std::string m_sGraphicsOutFolder;

	/**
	 * \brief For graphics, the step size for output.
	 */
	static double m_dGraphicsStepKM;

	/**
	 * \brief For graphics, the number of steps from hypocenter.
	 */
	static int m_iGraphicsSteps;

	/**
	 * \brief Maximum number of processing cycles a hypo can do without having
	 * new data associated
	 */
	static int m_iProcessLimit;

	/**
	 * \brief boolean to use a locator which minimizes TT as opposed to
	 * maximizes significance functions
	 */
	static bool m_bMinimizeTTLocator;

	/**
	 * \brief number of data required for reporting a hypo
	 */
	static int m_iReportingDataThreshold;

	/**
	 * \brief A double value containing the default viability threshold needed
	 * to for reporting a hypo
	 */
	static double m_dReportingStackThreshold;

	/**
	 * \brief An IGlassSend interface pointer used to send communication
	 * (such as output data), to outside the glasscore library
	 */
	static glasscore::IGlassSend *piSend;

	/**
	 * \brief A pointer to a CWeb object containing the detection web
	 */
	static CWebList * m_pWebList;

	/**
	 * \brief A pointer to a CSiteList object containing all the sites
	 * known to glasscore
	 */
	static CSiteList * m_pSiteList;

	/**
	 * \brief A pointer to a CPickList object containing the last n picks sent
	 * into glasscore (as determined by nPickMax).  Picks passed into pPickList
	 * are also passed into the pWeb detection web
	 */
	static CPickList * m_pPickList;

	/**
	 * \brief A pointer to a CPickList object containing the last n hypos sent
	 * into glasscore (as determined by nPickMax)
	 */
	static CHypoList * m_pHypoList;

	/**
	 * \brief A pointer to a CCorrelationList object containing the last n
	 * correlations sent into glasscore
	 */
	static CCorrelationList * m_pCorrelationList;

	/**
	 * \brief A pointer to a CDetection object used to process detections sent
	 * into glasscore
	 */
	static CDetection * m_pDetectionProcessor;

	/**
	 * \brief A pointer to a CTravelTime object containing
	 *default travel time for nucleation
	 */
	static std::shared_ptr<traveltime::CTravelTime> m_pDefaultNucleationTravelTime;

	/**
	 * \brief A pointer to a CTTT object containing the travel
	 * time phases and branches used by glasscore for association
	 */
	static std::shared_ptr<traveltime::CTTT> m_pAssociationTravelTimes;

	/**
	 * \brief the std::mutex for traveltimes
	 */
	static std::mutex m_TTTMutex;
};
}  // namespace glasscore
#endif  // GLASS_H
