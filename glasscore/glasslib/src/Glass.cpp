#include "Glass.h"
#include <json.h>
#include <date.h>
#include <geo.h>
#include <logger.h>
#include <TTT.h>
#include <TravelTime.h>
#include <cmath>
#include <string>
#include <atomic>
#include <memory>
#include "IGlassSend.h"
#include "WebList.h"
#include "SiteList.h"
#include "PickList.h"
#include "HypoList.h"
#include "CorrelationList.h"
#include "Detection.h"

namespace glasscore {

// pointers
glasscore::IGlassSend * CGlass::m_pExternalInterface = NULL;
CWebList * CGlass::m_pWebList = NULL;
CSiteList * CGlass::m_pSiteList = NULL;
CPickList * CGlass::m_pPickList = NULL;
CHypoList * CGlass::m_pHypoList = NULL;
CCorrelationList * CGlass::m_pCorrelationList = NULL;
CDetection * CGlass::m_pDetectionProcessor = NULL;
std::shared_ptr<traveltime::CTravelTime> CGlass::m_pDefaultNucleationTravelTime = // NOLINT
		NULL;
std::shared_ptr<traveltime::CTTT> CGlass::m_pAssociationTravelTimes = NULL;

// configuration values
std::atomic<int> CGlass::m_iMaxNumPicks;
std::atomic<int> CGlass::m_iMaxNumCorrelations;
std::atomic<int> CGlass::m_iMaxNumHypos;
std::atomic<int> CGlass::m_iNucleationDataCountThreshold;
std::atomic<int> CGlass::m_iNumStationsPerNode;
std::atomic<double> CGlass::m_dNucleationStackThreshold;
std::atomic<double> CGlass::m_dAssociationSDCutoff;
std::atomic<double> CGlass::m_dPruningSDCutoff;
std::atomic<double> CGlass::m_dPickAffinityExpFactor;
std::atomic<double> CGlass::m_dDistanceCutoffFactor;
std::atomic<double> CGlass::m_dDistanceCutoffRatio;
std::atomic<double> CGlass::m_dMinDistanceCutoff;
std::atomic<int> CGlass::m_iProcessLimit;
std::atomic<bool> CGlass::m_bTestTravelTimes;
std::atomic<bool> CGlass::m_bTestLocator;
std::atomic<bool> CGlass::m_bGraphicsOut;
std::string CGlass::m_sGraphicsOutFolder;  // NOLINT
std::atomic<double> CGlass::m_dGraphicsStepKM;
std::atomic<int> CGlass::m_iGraphicsSteps;
std::atomic<bool> CGlass::m_bMinimizeTTLocator;
std::atomic<double> CGlass::m_dPickDuplicateTimeWindow;
std::atomic<double> CGlass::m_dCorrelationMatchingTimeWindow;
std::atomic<double> CGlass::m_dCorrelationMatchingDistanceWindow;
std::atomic<int> CGlass::m_iCorrelationCancelAge;
std::atomic<double> CGlass::m_dBeamMatchingAzimuthWindow;
std::atomic<double> CGlass::m_dBeamMatchingDistanceWindow;
std::atomic<int> CGlass::m_iReportingDataThreshold;
std::atomic<double> CGlass::m_dReportingStackThreshold;
std::atomic<double> CGlass::m_dHypoMergingTimeWindow;
std::atomic<double> CGlass::m_dHypoMergingDistanceWindow;
std::atomic<double> CGlass::m_dEventFragmentDepthThreshold;
std::atomic<double> CGlass::m_dEventFragmentAzimuthThreshold;
std::atomic<bool> CGlass::m_bAllowPickUpdates;
std::mutex CGlass::m_TTTMutex;

// constants
// Related to Taper Range array
const unsigned int CGlass::k_nRangeArraySize;
const unsigned int CGlass::k_iTaperUpStart;
const unsigned int CGlass::k_iFullStart;
const unsigned int CGlass::k_iFullEnd;
const unsigned int CGlass::k_iTaperDownEnd;
// Related to Assoc (non-taper) range array
const unsigned int CGlass::k_nAssocArraySize;
const unsigned int CGlass::k_iAssocRangeStart;
const unsigned int CGlass::k_iAssocRangeEnd;
// Default Thread count values
const unsigned int CGlass::k_DefaultNumNucleationThreads;
const unsigned int CGlass::k_DefaultNumHypoThreads;
const unsigned int CGlass::k_DefaultNumWebThreads;
// Other default values
const int CGlass::k_nParamDisabled;  // used to indicate a param is disabled
const int CGlass::k_DefaultHoursWithoutPicking;
const int CGlass::k_DefaultHoursBeforeLookingUp;
const int CGlass::k_DefaultMaxPicksPerHour;
constexpr double CGlass::k_dAssociationSecondsPerSigma;
constexpr double CGlass::k_dNucleationSecondsPerSigma;
constexpr double CGlass::k_dMaximumDepth;

// ---------------------------------------------------------CGlass
CGlass::CGlass() {
	m_pExternalInterface = NULL;
	m_pWebList = NULL;
	m_pSiteList = NULL;
	m_pPickList = NULL;
	m_pHypoList = NULL;
	m_pCorrelationList = NULL;
	m_pDetectionProcessor = NULL;
	m_pDefaultNucleationTravelTime = NULL;
	m_pAssociationTravelTimes = NULL;

	clear();
}

// ---------------------------------------------------------~CGlass
CGlass::~CGlass() {
	if (m_pWebList) {
		delete (m_pWebList);
	}
	if (m_pSiteList) {
		delete (m_pSiteList);
	}
	if (m_pPickList) {
		delete (m_pPickList);
	}
	if (m_pHypoList) {
		delete (m_pHypoList);
	}
	if (m_pCorrelationList) {
		delete (m_pCorrelationList);
	}
	if (m_pDetectionProcessor) {
		delete (m_pDetectionProcessor);
	}
}

// -------------------------------------------------------receiveExternalMessage
bool CGlass::receiveExternalMessage(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error",
				"CGlass::receiveExternalMessage: NULL json communication.");
		return (false);
	}

	// check for a command, usually configuration or a request
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// Initialize glass
		if (v == "Initialize") {
			return (initialize(com));
		}

		// send any other commands to any of the configurable / input classes
		if (m_pSiteList->receiveExternalMessage(com)) {
			return (true);
		}
		if (m_pHypoList->receiveExternalMessage(com)) {
			return (true);
		}
		if (m_pWebList->receiveExternalMessage(com)) {
			return (true);
		}
	}

	// Check for a type, usually input data
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		// send any input to any of the input processing classes
		if (m_pPickList->receiveExternalMessage(com)) {
			return (true);
		}
		if (m_pSiteList->receiveExternalMessage(com)) {
			return (true);
		}
		if (m_pCorrelationList->receiveExternalMessage(com)) {
			return (true);
		}
		if (m_pDetectionProcessor->receiveExternalMessage(com)) {
			return (true);
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------setSend
void CGlass::setExternalInterface(glasscore::IGlassSend *newSend) {
	m_pExternalInterface = newSend;
}

// ---------------------------------------------------------send
bool CGlass::sendExternalMessage(std::shared_ptr<json::Object> com) {
	// make sure we have something to send to
	if (m_pExternalInterface) {
		// send the communication
		m_pExternalInterface->recieveGlassMessage(com);

		// done
		return (true);
	}

	// communication not sent
	return (false);
}

// ---------------------------------------------------------Clear
void CGlass::clear() {
	// reset to defaults
	m_iMaxNumPicks = -1;
	m_iMaxNumCorrelations = -1;
	m_iMaxNumHypos = -1;
	m_iNucleationDataCountThreshold = 7;
	m_iNumStationsPerNode = 20;
	m_dNucleationStackThreshold = 2.5;
	m_dAssociationSDCutoff = 3.0;
	m_dPruningSDCutoff = 3.0;
	m_dPickAffinityExpFactor = 2.5;
	m_dDistanceCutoffFactor = 4.0;
	m_dDistanceCutoffRatio = 0.4;
	m_dMinDistanceCutoff = 30.0;
	m_iProcessLimit = 25;
	m_bTestTravelTimes = false;
	m_bTestLocator = false;
	m_bGraphicsOut = false;
	m_sGraphicsOutFolder = "./";
	m_dGraphicsStepKM = 1.;
	m_iGraphicsSteps = 100;
	m_bMinimizeTTLocator = false;
	m_dPickDuplicateTimeWindow = 2.5;
	m_dCorrelationMatchingTimeWindow = 2.5;
	m_dCorrelationMatchingDistanceWindow = 0.5;
	m_iCorrelationCancelAge = 900;
	m_dBeamMatchingAzimuthWindow = 22.5;
	m_dBeamMatchingDistanceWindow = 5;
	m_iReportingDataThreshold = 0;
	m_dReportingStackThreshold = 2.5;
	m_dHypoMergingTimeWindow = 30.0;
	m_dHypoMergingDistanceWindow = 3.0;
	m_dEventFragmentDepthThreshold = 550.0;
	m_dEventFragmentAzimuthThreshold = 270.0;
	m_bAllowPickUpdates = false;
}

// ---------------------------------------------------------Initialize
bool CGlass::initialize(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log("error", "CGlass::initialize: NULL json.");
		return (false);
	}

	// check cmd
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		std::string cmd = (*com)["Cmd"].ToString();

		if (cmd != "Initialize") {
			glass3::util::Logger::log(
					"warning",
					"CGlass::initialize: Non-Initialize Cmd passed in.");
			return (false);
		}
	} else {
		// no command or type
		glass3::util::Logger::log(
				"error", "CGlass::initialize: Missing required Cmd Key.");
		return (false);
	}

	// Reset parameters
	clear();

	// set up travel times
	// load the first travel time
	if ((com->HasKey("DefaultNucleationPhase"))
			&& ((*com)["DefaultNucleationPhase"].GetType()
					== json::ValueType::ObjectVal)) {
		std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
		// get the phase object
		json::Object phsObj = (*com)["DefaultNucleationPhase"].ToObject();

		// clean out old phase if any
		m_pDefaultNucleationTravelTime.reset();

		// create new traveltime
		m_pDefaultNucleationTravelTime = std::make_shared<
				traveltime::CTravelTime>();

		// get the phase name
		// default to P
		std::string phs = "P";
		if (phsObj.HasKey("PhaseName")) {
			phs = phsObj["PhaseName"].ToString();
		}

		// get the file if present
		std::string file = "";
		if (phsObj.HasKey("TravFile")) {
			file = phsObj["TravFile"].ToString();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using file location: " + file
							+ " for default nucleation phase: " + phs);
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default file location for "
							" default nucleation phase: " + phs);
		}

		// set up the first phase travel time
		m_pDefaultNucleationTravelTime->setup(phs, file);

	} else {
		std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
		// if no first phase, default to P
		// clean out old phase if any
		m_pDefaultNucleationTravelTime.reset();

		// create new travel time
		m_pDefaultNucleationTravelTime = std::make_shared<
				traveltime::CTravelTime>();

		// set up the first phase travel time
		m_pDefaultNucleationTravelTime->setup("P");

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using  default nucleation first phase P");
	}

	// second, association phases
	if ((com->HasKey("AssociationPhases"))
			&& ((*com)["AssociationPhases"].GetType()
					== json::ValueType::ArrayVal)) {
		std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
		// get the array of phase entries
		json::Array phases = (*com)["AssociationPhases"].ToArray();

		// create and initialize travel time list
		m_pAssociationTravelTimes = std::make_shared<traveltime::CTTT>();

		// for each phase in the array
		for (auto val : phases) {
			// make sure the phase is an object
			if (val.GetType() != json::ValueType::ObjectVal) {
				continue;
			}

			// get this phase object
			json::Object obj = val.ToObject();

			double range[k_nRangeArraySize];  // NOLINT
			double *pdRange = NULL;
			double assoc[k_nAssocArraySize];  // NOLINT
			double * pdAssoc = NULL;
			std::string file = "";

			// get the phase name
			std::string phs = obj["PhaseName"].ToString();
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using association phase: " + phs);

			// get the Range if present, otherwise look for an Assoc
			if (obj.HasKey("Range")
					&& (obj["Range"].GetType() == json::ValueType::ArrayVal)) {
				// get the range array
				json::Array arr = obj["Range"].ToArray();

				// make sure the range array has the correct number of entries
				if (arr.size() != k_nRangeArraySize) {
					continue;
				}

				// copy out the range values
				for (int i = 0; i < k_nRangeArraySize; i++) {
					range[i] = arr[i].ToDouble();
				}

				glass3::util::Logger::log(
						"info",
						"CGlass::initialize: Using association Range = ["
								+ std::to_string(range[k_iTaperUpStart]) + ","
								+ std::to_string(range[k_iFullStart]) + ","
								+ std::to_string(range[k_iFullEnd]) + ","
								+ std::to_string(range[k_iTaperDownEnd]) + "]");
				glass3::util::Logger::log(
						"info",
						"CGlass::initialize: Using association Assoc = ["
								+ std::to_string(assoc[k_iAssocRangeStart])
								+ "," + std::to_string(assoc[k_iAssocRangeEnd])
								+ "]");

				// set range pointer
				pdRange = range;

				// populate assoc from range
				assoc[0] = range[0];
				assoc[1] = range[3];

				// set assoc pointer
				pdAssoc = assoc;

			} else if (obj.HasKey("Assoc")
					&& (obj["Assoc"].GetType() == json::ValueType::ArrayVal)) {
				// get the assoc array
				json::Array arr = obj["Assoc"].ToArray();

				// make sure the assoc array has the correct number of entries
				if (arr.size() != k_nAssocArraySize) {
					continue;
				}

				// copy out the assoc values
				for (int i = 0; i < k_nAssocArraySize; i++) {
					assoc[i] = arr[i].ToDouble();
				}

				glass3::util::Logger::log(
						"info",
						"CGlass::initialize: Using association Assoc = ["
								+ std::to_string(assoc[k_iAssocRangeStart])
								+ "," + std::to_string(assoc[k_iAssocRangeEnd])
								+ "]");

				// set range pointer
				pdRange = NULL;

				// set assoc pointer
				pdAssoc = assoc;
			} else {
				glass3::util::Logger::log(
						"error",
						"CGlass::initialize: Missing required Range or Assoc key.");
				continue;
			}

			// get the travel time file if present
			if (obj.HasKey("TravFile")
					&& (obj["TravFile"].GetType() == json::ValueType::StringVal)) {
				file = obj["TravFile"].ToString();

				glass3::util::Logger::log(
						"info",
						"CGlass::initialize: Using association tt file: "
								+ file);
			} else {
				glass3::util::Logger::log(
						"info",
						"CGlass::initialize: Using default file location for "
								"association phase: " + phs);
			}

			// set up this phase
			m_pAssociationTravelTimes->addPhase(phs, pdRange, pdAssoc, file);

			// test this phase
			if (m_bTestTravelTimes) {
				m_pAssociationTravelTimes->testTravelTimes(phs);
			}
		}
	} else {
		glass3::util::Logger::log("error",
									"No association Phase array provided");
		return (false);
	}

	// setup if we are going to print phase travel times for debuging
	if ((com->HasKey("TestTravelTimes"))
			&& ((*com)["TestTravelTimes"].GetType() == json::ValueType::BoolVal)) {
		m_bTestTravelTimes = (*com)["TestTravelTimes"].ToBool();
	}

	// Change locator
	if ((com->HasKey("UseL1ResidualLocator"))
			&& ((*com)["UseL1ResidualLocator"].GetType()
					== json::ValueType::BoolVal)) {
		m_bMinimizeTTLocator = (*com)["UseL1ResidualLocator"].ToBool();
	}

	// Collect info for files to plot output
	if ((com->HasKey("PlottingInfo"))
			&& ((*com)["PlottingInfo"].GetType() == json::ValueType::ObjectVal)) {
		json::Object paramsPlot = (*com)["PlottingInfo"].ToObject();

		if ((paramsPlot.HasKey("graphicsOut"))
				&& (paramsPlot["graphicsOut"].GetType()
						== json::ValueType::BoolVal)) {
			m_bGraphicsOut = paramsPlot["graphicsOut"].ToBool();
			if (m_bGraphicsOut == true) {
				glass3::util::Logger::log(
						"info", "CGlass::initialize: Plotting output is on!!!");
			}

			if (m_bGraphicsOut == false) {
				glass3::util::Logger::log(
						"info", "CGlass::initialize: Plotting output is off.");
			}
		}

		if ((paramsPlot.HasKey("graphicsStepKM"))
				&& (paramsPlot["graphicsStepKM"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dGraphicsStepKM = paramsPlot["graphicsStepKM"].ToDouble();
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Plotting Step Increment: "
							+ std::to_string(m_dGraphicsStepKM));
		}

		if ((paramsPlot.HasKey("graphicsSteps"))
				&& (paramsPlot["graphicsSteps"].GetType()
						== json::ValueType::IntVal)) {
			m_iGraphicsSteps = paramsPlot["graphicsSteps"].ToInt();
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Plotting Steps: "
							+ std::to_string(m_iGraphicsSteps));
		}

		if ((paramsPlot.HasKey("graphicsOutFolder"))
				&& (paramsPlot["graphicsOutFolder"].GetType()
						== json::ValueType::StringVal)) {
			std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
			m_sGraphicsOutFolder = paramsPlot["graphicsOutFolder"].ToString();
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Plotting Output Location: "
							+ m_sGraphicsOutFolder);
		}
	}

	// Collect association and nucleation tuning parameters
	if ((com->HasKey("Params"))
			&& ((*com)["Params"].GetType() == json::ValueType::ObjectVal)) {
		// get object
		json::Object params = (*com)["Params"].ToObject();

		// Thresh
		if ((params.HasKey("NucleationStackThreshold"))
				&& (params["NucleationStackThreshold"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dNucleationStackThreshold = params["NucleationStackThreshold"]
					.ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using NucleationStackThreshold: "
							+ std::to_string(m_dNucleationStackThreshold));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default NucleationStackThreshold: "
							+ std::to_string(m_dNucleationStackThreshold));
		}

		// Nucleate
		if ((params.HasKey("NucleationDataCountThreshold"))
				&& (params["NucleationDataCountThreshold"].GetType()
						== json::ValueType::IntVal)) {
			m_iNucleationDataCountThreshold =
					params["NucleationDataCountThreshold"].ToInt();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using NucleationDataCountThreshold: "
							+ std::to_string(m_iNucleationDataCountThreshold));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default NucleationDataCountThreshold: "
							+ std::to_string(m_iNucleationDataCountThreshold));
		}

		// sdAssociate
		if ((params.HasKey("AssociationStandardDeviationCutoff"))
				&& (params["AssociationStandardDeviationCutoff"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dAssociationSDCutoff =
					params["AssociationStandardDeviationCutoff"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using AssociationStandardDeviationCutoff: "
							+ std::to_string(m_dAssociationSDCutoff));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default AssociationStandardDeviationCutoff: "
							+ std::to_string(m_dAssociationSDCutoff));
		}

		// sdPrune
		if ((params.HasKey("PruningStandardDeviationCutoff"))
				&& (params["PruningStandardDeviationCutoff"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dPruningSDCutoff = params["PruningStandardDeviationCutoff"]
					.ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using PruningStandardDeviationCutoff: "
							+ std::to_string(m_dPruningSDCutoff));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default PruningStandardDeviationCutoff: "
							+ std::to_string(m_dPruningSDCutoff));
		}

		// ExpAffinity
		if ((params.HasKey("PickAffinityExponentialFactor"))
				&& (params["PickAffinityExponentialFactor"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dPickAffinityExpFactor = params["PickAffinityExponentialFactor"]
					.ToDouble();
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using PickAffinityExponentialFactor: "
							+ std::to_string(m_dPickAffinityExpFactor));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default PickAffinityExponentialFactor: "
							+ std::to_string(m_dPickAffinityExpFactor));
		}

		// m_dDistanceCutoffFactor
		if ((params.HasKey("DistanceCutoffFactor"))
				&& (params["DistanceCutoffFactor"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dDistanceCutoffFactor = params["DistanceCutoffFactor"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using DistanceCutoffFactor: "
							+ std::to_string(m_dDistanceCutoffFactor));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default DistanceCutoffFactor: "
							+ std::to_string(m_dDistanceCutoffFactor));
		}

		// m_dDistanceCutoffRatio
		if ((params.HasKey("DistanceCutoffRatio"))
				&& (params["DistanceCutoffRatio"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dDistanceCutoffRatio = params["DistanceCutoffRatio"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using DistanceCutoffRatio: "
							+ std::to_string(m_dDistanceCutoffRatio));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default DistanceCutoffRatio: "
							+ std::to_string(m_dDistanceCutoffRatio));
		}

		// m_dMinDistanceCutoff
		if ((params.HasKey("DistanceCutoffMinimum"))
				&& (params["DistanceCutoffMinimum"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dMinDistanceCutoff = params["DistanceCutoffMinimum"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using DistanceCutoffMinimum: "
							+ std::to_string(m_dMinDistanceCutoff));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default DistanceCutoffMinimum: "
							+ std::to_string(m_dMinDistanceCutoff));
		}

		// iCycleLimit
		if ((params.HasKey("HypoProcessCountLimit"))
				&& (params["HypoProcessCountLimit"].GetType()
						== json::ValueType::IntVal)) {
			m_iProcessLimit = params["HypoProcessCountLimit"].ToInt();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using HypoProcessCountLimit: "
							+ std::to_string(m_iProcessLimit));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default HypoProcessCountLimit: "
							+ std::to_string(m_iProcessLimit));
		}

		// CorrelationTimeWindow
		if ((params.HasKey("CorrelationTimeWindow"))
				&& (params["CorrelationTimeWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dCorrelationMatchingTimeWindow = params["CorrelationTimeWindow"]
					.ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using CorrelationTimeWindow: "
							+ std::to_string(m_dCorrelationMatchingTimeWindow));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default "
							"CorrelationTimeWindow: "
							+ std::to_string(m_dCorrelationMatchingTimeWindow));
		}

		// CorrelationDistanceWindow
		if ((params.HasKey("CorrelationDistanceWindow"))
				&& (params["CorrelationDistanceWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dCorrelationMatchingDistanceWindow =
					params["CorrelationDistanceWindow"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using CorrelationDistanceWindow: "
							+ std::to_string(
									m_dCorrelationMatchingDistanceWindow));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default "
							"CorrelationDistanceWindow: "
							+ std::to_string(
									m_dCorrelationMatchingDistanceWindow));
		}

		// correlationCancelAge
		if ((params.HasKey("CorrelationCancelAge"))
				&& (params["CorrelationCancelAge"].GetType()
						== json::ValueType::DoubleVal)) {
			m_iCorrelationCancelAge = params["CorrelationCancelAge"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using CorrelationCancelAge: "
							+ std::to_string(m_iCorrelationCancelAge));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default CorrelationCancelAge: "
							+ std::to_string(m_iCorrelationCancelAge));
		}

		// HypocenterTimeWindow
		if ((params.HasKey("HypocenterTimeWindow"))
				&& (params["HypocenterTimeWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dHypoMergingTimeWindow =
					params["HypocenterTimeWindow"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using HypocenterTimeWindow: "
							+ std::to_string(m_dHypoMergingTimeWindow));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default "
							"HypocenterTimeWindow: "
							+ std::to_string(m_dHypoMergingTimeWindow));
		}

		// HypocenterDistanceWindow
		if ((params.HasKey("HypocenterDistanceWindow"))
				&& (params["HypocenterDistanceWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dHypoMergingDistanceWindow = params["HypocenterDistanceWindow"]
					.ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using HypocenterDistanceWindow: "
							+ std::to_string(m_dHypoMergingDistanceWindow));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default "
							"HypocenterDistanceWindow: "
							+ std::to_string(m_dHypoMergingDistanceWindow));
		}

		// beamMatchingAzimuthWindow
		if ((params.HasKey("BeamMatchingAzimuthWindow"))
				&& (params["BeamMatchingAzimuthWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dBeamMatchingAzimuthWindow = params["BeamMatchingAzimuthWindow"]
					.ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using BeamMatchingAzimuthWindow: "
							+ std::to_string(m_dBeamMatchingAzimuthWindow));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default "
							"BeamMatchingAzimuthWindow: "
							+ std::to_string(m_dBeamMatchingAzimuthWindow));
		}

		// EventFragmentDepthThreshold
		if ((params.HasKey("EventFragmentDepthThreshold"))
				&& (params["EventFragmentDepthThreshold"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dEventFragmentDepthThreshold =
					params["EventFragmentDepthThreshold"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using EventFragmentDepthThreshold: "
							+ std::to_string(m_dEventFragmentDepthThreshold));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default "
							"EventFragmentDepthThreshold: "
							+ std::to_string(m_dEventFragmentDepthThreshold));
		}

		// EventFragmentAzimuthThreshold
		if ((params.HasKey("EventFragmentAzimuthThreshold"))
				&& (params["EventFragmentAzimuthThreshold"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dEventFragmentAzimuthThreshold =
					params["EventFragmentAzimuthThreshold"].ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using EventFragmentAzimuthThreshold: "
							+ std::to_string(m_dEventFragmentAzimuthThreshold));
		} else {
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default "
							"EventFragmentAzimuthThreshold: "
							+ std::to_string(m_dEventFragmentAzimuthThreshold));
		}

		/*
		 * NOTE: Keep for when beams are fully implemented
		 // beamMatchingDistanceWindow
		 if ((params.HasKey("dBeamMatchingDistanceWindow"))
		 && (params["dBeamMatchingDistanceWindow"].GetType()
		 == json::ValueType::DoubleVal)) {
		 beamMatchingDistanceWindow = params["dBeamMatchingDistanceWindow"]
		 .ToDouble();

		 glass3::util::Logger::log(
		 "info",
		 "CGlass::initialize: Using dBeamMatchingDistanceWindow: "
		 + std::to_string(beamMatchingDistanceWindow));
		 } else {
		 glass3::util::Logger::log(
		 "info",
		 "CGlass::initialize: Using default "
		 "dBeamMatchingDistanceWindow: "
		 + std::to_string(beamMatchingDistanceWindow));
		 }
		 */

		// dReportThresh
		if ((params.HasKey("ReportingStackThreshold"))
				&& (params["ReportingStackThreshold"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dReportingStackThreshold = params["ReportingStackThreshold"]
					.ToDouble();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using ReportingStackThreshold: "
							+ std::to_string(m_dReportingStackThreshold));
		} else {
			// default to overall thresh
			m_dReportingStackThreshold = getNucleationStackThreshold();
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default ReportingStackThreshold "
							"( = NucleationStackThreshold): "
							+ std::to_string(m_dReportingStackThreshold));
		}

		// nReportCut
		if ((params.HasKey("ReportingDataThreshold"))
				&& (params["ReportingDataThreshold"].GetType()
						== json::ValueType::IntVal)) {
			m_iReportingDataThreshold =
					params["ReportingDataThreshold"].ToInt();

			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using ReportingDataThreshold: "
							+ std::to_string(m_iReportingDataThreshold));
		} else {
			// default to overall nNucleate
			m_iReportingDataThreshold = getNucleationDataCountThreshold();
			glass3::util::Logger::log(
					"info",
					"CGlass::initialize: Using default ReportingDataThreshold: "
							+ std::to_string(m_iReportingDataThreshold));
		}
	} else {
		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using default association and nucleation "
				"parameters");
	}

	// Test Locator
	if ((com->HasKey("TestLocator"))
			&& ((*com)["TestLocator"].GetType() == json::ValueType::BoolVal)) {
		m_bTestLocator = (*com)["TestLocator"].ToBool();

		if (m_bTestLocator) {
			glass3::util::Logger::log(
					"info", "CGlass::initialize: testLocator set to true");
		} else {
			glass3::util::Logger::log(
					"info", "CGlass::initialize: testLocator set to false");
		}
	} else {
		glass3::util::Logger::log(
				"info", "CGlass::initialize: testLocator not Found!");
	}

	// set maximum number of picks
	if ((com->HasKey("MaximumNumberOfPicks"))
			&& ((*com)["MaximumNumberOfPicks"].GetType()
					== json::ValueType::IntVal)) {
		m_iMaxNumPicks = (*com)["MaximumNumberOfPicks"].ToInt();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using MaximumNumberOfPicks: "
						+ std::to_string(m_iMaxNumPicks));
	}

	// set maximum number of correlations
	if ((com->HasKey("MaximumNumberOfCorrelations"))
			&& ((*com)["MaximumNumberOfCorrelations"].GetType()
					== json::ValueType::IntVal)) {
		m_iMaxNumCorrelations = (*com)["MaximumNumberOfCorrelations"].ToInt();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using MaximumNumberOfCorrelations: "
						+ std::to_string(m_iMaxNumCorrelations));
	}

	// set maximum number of hypos
	if ((com->HasKey("MaximumNumberOfHypos"))
			&& ((*com)["MaximumNumberOfHypos"].GetType()
					== json::ValueType::IntVal)) {
		m_iMaxNumHypos = (*com)["MaximumNumberOfHypos"].ToInt();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using MaximumNumberOfHypos: "
						+ std::to_string(m_iMaxNumHypos));
	}

	// set pick duplicate window
	if ((com->HasKey("PickDuplicateWindow"))
			&& ((*com)["PickDuplicateWindow"].GetType()
					== json::ValueType::DoubleVal)) {
		m_dPickDuplicateTimeWindow = (*com)["PickDuplicateWindow"].ToDouble();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using PickDuplicateWindow: "
						+ std::to_string(m_dPickDuplicateTimeWindow));
	} else {
		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using default PickDuplicateWindow: "
						+ std::to_string(m_dPickDuplicateTimeWindow));
	}

	// set the number of nucleation threads
	int numNucleationThreads = k_DefaultNumNucleationThreads;
	if ((com->HasKey("NumberOfNucleationThreads"))
			&& ((*com)["NumberOfNucleationThreads"].GetType()
					== json::ValueType::IntVal)) {
		numNucleationThreads = (*com)["NumberOfNucleationThreads"].ToInt();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using NumberOfNucleationThreads: "
						+ std::to_string(numNucleationThreads));
	} else {
		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using default NumberOfNucleationThreads: "
						+ std::to_string(numNucleationThreads));
	}

	// set the number of hypo threads
	int numHypoThreads = k_DefaultNumNucleationThreads;
	if ((com->HasKey("NumberOfHypoThreads"))
			&& ((*com)["NumberOfHypoThreads"].GetType()
					== json::ValueType::IntVal)) {
		numHypoThreads = (*com)["NumberOfHypoThreads"].ToInt();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using NumberOfHypoThreads: "
						+ std::to_string(numHypoThreads));
	} else {
		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using default NumberOfHypoThreads: "
						+ std::to_string(numHypoThreads));
	}

	// set the number of web threads
	int numWebThreads = k_DefaultNumWebThreads;
	if ((com->HasKey("NumberOfWebThreads"))
			&& ((*com)["NumberOfWebThreads"].GetType()
					== json::ValueType::IntVal)) {
		numWebThreads = (*com)["NumberOfWebThreads"].ToInt();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using NumberOfWebThreads: "
						+ std::to_string(numWebThreads));
	} else {
		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using default NumberOfWebThreads: "
						+ std::to_string(numWebThreads));
	}

	int iHoursWithoutPicking = k_DefaultHoursWithoutPicking;
	if ((com->HasKey("SiteHoursWithoutPicking"))
			&& ((*com)["SiteHoursWithoutPicking"].GetType()
					== json::ValueType::IntVal)) {
		iHoursWithoutPicking = (*com)["SiteHoursWithoutPicking"].ToInt();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using SiteHoursWithoutPicking: "
						+ std::to_string(iHoursWithoutPicking));
	} else {
		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using default SiteHoursWithoutPicking: "
						+ std::to_string(iHoursWithoutPicking));
	}

	int iHoursBeforeLookingUp = k_DefaultHoursBeforeLookingUp;
	if ((com->HasKey("SiteLookupInterval"))
			&& ((*com)["SiteLookupInterval"].GetType()
					== json::ValueType::IntVal)) {
		iHoursBeforeLookingUp = (*com)["SiteLookupInterval"].ToInt();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using SiteLookupInterval: "
						+ std::to_string(iHoursBeforeLookingUp));
	} else {
		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using default SiteLookupInterval: "
						+ std::to_string(iHoursBeforeLookingUp));
	}

	int iMaxPicksPerHour = k_DefaultMaxPicksPerHour;
	if ((com->HasKey("SiteMaximumPicksPerHour"))
			&& ((*com)["SiteMaximumPicksPerHour"].GetType()
					== json::ValueType::IntVal)) {
		iMaxPicksPerHour = (*com)["SiteMaximumPicksPerHour"].ToInt();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using SiteMaximumPicksPerHour: "
						+ std::to_string(iMaxPicksPerHour));
	} else {
		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using default SiteMaximumPicksPerHour: "
						+ std::to_string(iMaxPicksPerHour));
	}

	// set whether to allow pick updates
	if ((com->HasKey("AllowPickUpdates"))
			&& ((*com)["AllowPickUpdates"].GetType() == json::ValueType::BoolVal)) {  // NOLINT
		m_bAllowPickUpdates = (*com)["AllowPickUpdates"].ToBool();

		glass3::util::Logger::log(
				"info",
				"CGlass::initialize: Using AllowPickUpdates: "
						+ std::to_string(m_bAllowPickUpdates));
	}

	// create site list
	if (m_pSiteList == NULL) {
		m_pSiteList = new CSiteList();
	}
	m_pSiteList->setMaxHoursWithoutPicking(iHoursWithoutPicking);
	m_pSiteList->setHoursBeforeLookingUp(iHoursBeforeLookingUp);
	m_pSiteList->setMaxPicksPerHour(iMaxPicksPerHour);

	// create web list
	if (m_pWebList == NULL) {
		m_pWebList = new CWebList(numWebThreads);
	}
	m_pWebList->setSiteList(m_pSiteList);

	// create pick list
	if (m_pPickList == NULL) {
		m_pPickList = new CPickList(numNucleationThreads);
	}
	m_pPickList->setSiteList(m_pSiteList);

	// create correlation list
	if (m_pCorrelationList == NULL) {
		m_pCorrelationList = new CCorrelationList();
	}
	m_pPickList->setSiteList(m_pSiteList);

	// create hypo list
	if (m_pHypoList == NULL) {
		m_pHypoList = new CHypoList(numHypoThreads);
	}

	// create detection processor
	if (m_pDetectionProcessor == NULL) {
		m_pDetectionProcessor = new CDetection();
	}

	return (true);
}

// ---------------------------------------------------------statusCheck
bool CGlass::healthCheck() {
	// nullcheck
	if (m_pPickList == NULL) {
		return (false);
	}

	// check pick list
	if (m_pPickList->healthCheck() == false) {
		return (false);
	}

	// hypo list
	if (m_pHypoList->healthCheck() == false) {
		return (false);
	}

	// site list
	if (m_pSiteList->healthCheck() == false) {
		return (false);
	}

	// webs
	if (m_pWebList->healthCheck() == false) {
		return (false);
	}

	// all is well
	return (true);
}

// -------------------------------------------------getBeamMatchingAzimuthWindow
double CGlass::getBeamMatchingAzimuthWindow() {
	return (m_dBeamMatchingAzimuthWindow);
}

// ------------------------------------------------getBeamMatchingDistanceWindow
double CGlass::getBeamMatchingDistanceWindow() {
	return (m_dBeamMatchingDistanceWindow);
}

// ------------------------------------------------getCorrelationCancelAge
int CGlass::getCorrelationCancelAge() {
	return (m_iCorrelationCancelAge);
}

// ---------------------------------------------getCorrelationMatchingTimeWindow
double CGlass::getCorrelationMatchingTimeWindow() {
	return (m_dCorrelationMatchingTimeWindow);
}

// -----------------------------------------getCorrelationMatchingDistanceWindow
double CGlass::getCorrelationMatchingDistanceWindow() {
	return (m_dCorrelationMatchingDistanceWindow);
}

// ---------------------------------------------getHypoMergingTimeWindow
double CGlass::getHypoMergingTimeWindow() {
	return (m_dHypoMergingTimeWindow);
}

// -----------------------------------------getHypoMergingDistanceWindow
double CGlass::getHypoMergingDistanceWindow() {
	return (m_dHypoMergingDistanceWindow);
}

// ------------------------------------------------getDistanceCutoffFactor
double CGlass::getDistanceCutoffFactor() {
	return (m_dDistanceCutoffFactor);
}

// ------------------------------------------------getMinDistanceCutoff
double CGlass::getMinDistanceCutoff() {
	return (m_dMinDistanceCutoff);
}

// ------------------------------------------------getDistanceCutoffPercentage
double CGlass::getDistanceCutoffRatio() {
	return (m_dDistanceCutoffRatio);
}

// ------------------------------------------------getReportingStackThreshold
double CGlass::getReportingStackThreshold() {
	return (m_dReportingStackThreshold);
}

// ------------------------------------------------getNucleationStackThreshold
double CGlass::getNucleationStackThreshold() {
	return (m_dNucleationStackThreshold);
}

// ------------------------------------------------getPickAffinityExpFactor
double CGlass::getPickAffinityExpFactor() {
	return (m_dPickAffinityExpFactor);
}

// ------------------------------------------------getGraphicsOut
bool CGlass::getGraphicsOut() {
	return (m_bGraphicsOut);
}

// ------------------------------------------------getGraphicsOutFolder
const std::string& CGlass::getGraphicsOutFolder() {
	std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
	return (m_sGraphicsOutFolder);
}

// ------------------------------------------------getGraphicsStepKm
double CGlass::getGraphicsStepKm() {
	return (m_dGraphicsStepKM);
}

// ------------------------------------------------getGraphicsSteps
int CGlass::getGraphicsSteps() {
	return (m_iGraphicsSteps);
}

// ------------------------------------------------getCycleLimit
int CGlass::getProcessLimit() {
	return (m_iProcessLimit);
}

// ------------------------------------------------getMinimizeTTLocator
bool CGlass::getMinimizeTTLocator() {
	return (m_bMinimizeTTLocator);
}

// ------------------------------------------------setMinimizeTTLocator
void CGlass::setMinimizeTTLocator(bool use) {
	m_bMinimizeTTLocator = use;
}

// ------------------------------------------------getNumStationsPerNode
int CGlass::getNumStationsPerNode() {
	return (m_iNumStationsPerNode);
}

// ------------------------------------------------getNucleationDataThreshold
int CGlass::getNucleationDataCountThreshold() {
	return (m_iNucleationDataCountThreshold);
}

// ------------------------------------------------getReportingDataThreshold
int CGlass::getReportingDataThreshold() {
	return (m_iReportingDataThreshold);
}

// ------------------------------------------------getMaxNumPicks
int CGlass::getMaxNumPicks() {
	return (m_iMaxNumPicks);
}

// ------------------------------------------------setMaxNumPicks
void CGlass::setMaxNumPicks(int max) {
	m_iMaxNumPicks = max;
}

// ------------------------------------------------getMaxNumCorrelations
int CGlass::getMaxNumCorrelations() {
	return (m_iMaxNumCorrelations);
}

// ------------------------------------------------setMaxNumCorrelations
void CGlass::setMaxNumCorrelations(int max) {
	m_iMaxNumCorrelations = max;
}

// ------------------------------------------------getMaxNumHypos
int CGlass::getMaxNumHypos() {
	return (m_iMaxNumHypos);
}

// ------------------------------------------------setMaxNumHypos
void CGlass::setMaxNumHypos(int max) {
	m_iMaxNumHypos = max;
}

// ----------------------------------------------getEventFragmentDepthThreshold
double CGlass::getEventFragmentDepthThreshold() {
	return (m_dEventFragmentDepthThreshold);
}

// ---------------------------------------------getEventFragmentAzimuthThreshold
double CGlass::getEventFragmentAzimuthThreshold() {
	return (m_dEventFragmentAzimuthThreshold);
}

// ------------------------------------------------getCorrelationList
CCorrelationList* CGlass::getCorrelationList() {
	return (m_pCorrelationList);
}

// ------------------------------------------------getDetectionProcessor
CDetection* CGlass::getDetectionProcessor() {
	return (m_pDetectionProcessor);
}

// ------------------------------------------------getHypoList
CHypoList* CGlass::getHypoList() {
	return (m_pHypoList);
}

// ------------------------------------------------getPickDuplicateTimeWindow
double CGlass::getPickDuplicateTimeWindow() {
	return (m_dPickDuplicateTimeWindow);
}

// ------------------------------------------------getPickList
CPickList* CGlass::getPickList() {
	return (m_pPickList);
}

// ------------------------------------------------getSiteList
CSiteList* CGlass::getSiteList() {
	return (m_pSiteList);
}

// -----------------------------------------------getDefaultNucleationTravelTime
std::shared_ptr<traveltime::CTravelTime>& CGlass::getDefaultNucleationTravelTime() {  // NOLINT
	std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
	return (m_pDefaultNucleationTravelTime);
}

// ------------------------------------------------getAssociationTravelTimes
std::shared_ptr<traveltime::CTTT>& CGlass::getAssociationTravelTimes() {
	std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
	return (m_pAssociationTravelTimes);
}

// ------------------------------------------------getWebList
CWebList* CGlass::getWebList() {
	return (m_pWebList);
}

// ------------------------------------------------getAssociationSDCutoff
double CGlass::getAssociationSDCutoff() {
	return (m_dAssociationSDCutoff);
}

// ------------------------------------------------getPruningSDCutoff
double CGlass::getPruningSDCutoff() {
	return (m_dPruningSDCutoff);
}

// ------------------------------------------------getTestLocator
bool CGlass::getTestLocator() {
	return (m_bTestLocator);
}

// ------------------------------------------------getTestTravelTimes
bool CGlass::getTestTravelTimes() {
	return (m_bTestTravelTimes);
}

// ------------------------------------------------getAllowPickUpdates
bool CGlass::getAllowPickUpdates() {
	return (m_bAllowPickUpdates);
}

// Start correlation specific edits

// ------------------------------------------------getAllowPickUpdates
double CGlass::getMaxCorrelationValue() {
	return ( m_correlationMaximumValue);
}

// ------------------------------------------------getAllowPickUpdates
double CGlass::getCorrelationSpatialSTD() {
	return (m_correlationSpatialSTD);
}

// ------------------------------------------------getAllowPickUpdates
double CGlass::getMinimumCorrelationCoefficient() {
	return (m_minimumCorrelationCoefficient);
}

}  // namespace glasscore
