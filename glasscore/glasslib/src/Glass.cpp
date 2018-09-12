#include <json.h>
#include <cmath>
#include <string>
#include <atomic>
#include "IGlassSend.h"
#include "Date.h"
#include "Geo.h"
#include "Glass.h"
#include "WebList.h"
#include "SiteList.h"
#include "PickList.h"
#include "HypoList.h"
#include "CorrelationList.h"
#include "Detection.h"
#include "Terra.h"
#include "Trav.h"
#include "TTT.h"
#include "TravelTime.h"
#include "Logit.h"
#include <memory>

namespace glasscore {

glasscore::IGlassSend *CGlass::piSend = NULL;
CWebList* CGlass::m_pWebList = NULL;
CSiteList* CGlass::m_pSiteList = NULL;
CPickList* CGlass::m_pPickList = NULL;
CHypoList* CGlass::m_pHypoList = NULL;
CCorrelationList* CGlass::m_pCorrelationList = NULL;
CDetection* CGlass::m_pDetectionProcessor = NULL;
std::shared_ptr<traveltime::CTravelTime> CGlass::m_pDefaultNucleationTravelTime =  // NOLINT
		NULL;
std::shared_ptr<traveltime::CTTT> CGlass::m_pAssociationTravelTimes = NULL;

int CGlass::m_iMaxNumPicks = -1;
int CGlass::m_iMaxNumCorrelations = -1;
int CGlass::m_iMaxNumPicksPerSite = -1;
int CGlass::m_iMaxNumHypos = -1;
int CGlass::m_iNucleationDataThreshold = 7;
int CGlass::m_iNumStationsPerNode = 20;
double CGlass::m_dNucleationStackThreshold = 2.5;
double CGlass::m_dAssociationSDCutoff = 3.0;
double CGlass::m_dPruningSDCutoff = 3.0;
double CGlass::m_dPickAffinityExpFactor = 2.5;
double CGlass::m_dDistanceCutoffFactor = 4.0;
double CGlass::m_dDistanceCutoffPercentage = 0.4;
double CGlass::m_dMinDistanceCutoff = 30.0;
int CGlass::m_iProcessLimit = 25;
bool CGlass::m_bTestTravelTimes = false;
bool CGlass::m_bTestLocator = false;
bool CGlass::m_bGraphicsOut = false;
std::string CGlass::m_sGraphicsOutFolder = "./";  // NOLINT
double CGlass::m_dGraphicsStepKM = 1.;
int CGlass::m_iGraphicsSteps = 100;
bool CGlass::m_bMinimizeTTLocator = false;
double CGlass::m_dPickDuplicateTimeWindow = 2.5;
double CGlass::m_dCorrelationMatchingTimeWindow = 2.5;
double CGlass::m_dCorrelationMatchingDistanceWindow = .5;
int CGlass::m_iCorrelationCancelAge = 900;
double CGlass::m_dBeamMatchingAzimuthWindow = 22.5;
double CGlass::m_dBeamMatchingDistanceWindow = 5;
int CGlass::m_iReportingDataThreshold = 0;
double CGlass::m_dReportingStackThreshold = 2.5;
double CGlass::m_dHypoMergingTimeWindow = 30.0;
double CGlass::m_dHypoMergingDistanceWindow = 3.0;

std::mutex CGlass::m_TTTMutex;

// ---------------------------------------------------------CGlass
CGlass::CGlass() {
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

// ---------------------------------------------------------dispatch
bool CGlass::dispatch(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CGlass::dispatch: NULL json communication.");
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
		if (m_pSiteList->dispatch(com)) {
			return (true);
		}
		if (m_pHypoList->dispatch(com)) {
			return (true);
		}
		if (m_pWebList->dispatch(com)) {
			return (true);
		}
	}

	// Check for a type, usually input data
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		// send any input to any of the input processing classes
		if (m_pPickList->dispatch(com)) {
			return (true);
		}
		if (m_pSiteList->dispatch(com)) {
			return (true);
		}
		if (m_pCorrelationList->dispatch(com)) {
			return (true);
		}
		if (m_pDetectionProcessor->dispatch(com)) {
			return (true);
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------setSend
void CGlass::setSend(glasscore::IGlassSend *newSend) {
	piSend = newSend;
}

// ---------------------------------------------------------send
bool CGlass::send(std::shared_ptr<json::Object> com) {
	// make sure we have something to send to
	if (piSend) {
		// send the communication
		piSend->Send(com);

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
	m_iMaxNumPicksPerSite = -1;
	m_iMaxNumHypos = -1;
	m_iNucleationDataThreshold = 7;
	m_iNumStationsPerNode = 20;
	m_iNucleationDataThreshold = 7;
	m_dNucleationStackThreshold = 2.5;
	m_dAssociationSDCutoff = 3.0;
	m_dPruningSDCutoff = 3.0;
	m_dPickAffinityExpFactor = 2.5;
	m_dDistanceCutoffFactor = 4.0;
	m_dDistanceCutoffPercentage = 0.4;
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
	m_dCorrelationMatchingDistanceWindow = .5;
	m_iCorrelationCancelAge = 900;
	m_dBeamMatchingAzimuthWindow = 22.5;
	m_dBeamMatchingDistanceWindow = 5;
	m_iReportingDataThreshold = 0;
	m_dReportingStackThreshold = 2.5;
	m_dHypoMergingTimeWindow = 30.0;
	m_dHypoMergingDistanceWindow = 3.0;
}

// ---------------------------------------------------------Initialize
bool CGlass::initialize(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CGlass::initialize: NULL json.");
		return (false);
	}

	// check cmd
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		std::string cmd = (*com)["Cmd"].ToString();

		if (cmd != "Initialize") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CGlass::initialize: Non-Initialize Cmd passed in.");
			return (false);
		}
	} else {
		// no command or type
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CGlass::initialize: Missing required Cmd Key.");
		return (false);
	}

	// Reset parameters
	clear();

	// set up travel times
	// load the first travel time
	if ((com->HasKey("DefaultNucleationPhase"))
			&& ((*com)["DefaultNucleationPhase"].GetType()
					== json::ValueType::ObjectVal)) {
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

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using file location: " + file
							+ " for default nucleation phase: " + phs);
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default file location for "
							" default nucleation phase: " + phs);
		}

		// set up the first phase travel time
		m_pDefaultNucleationTravelTime->setup(phs, file);

	} else {
		// if no first phase, default to P
		// clean out old phase if any
		m_pDefaultNucleationTravelTime.reset();

		// create new travel time
		m_pDefaultNucleationTravelTime = std::make_shared<
				traveltime::CTravelTime>();

		// set up the first phase travel time
		m_pDefaultNucleationTravelTime->setup("P");

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using  default nucleation first phase P");
	}

	// second, association phases
	if ((com->HasKey("AssociationPhases"))
			&& ((*com)["AssociationPhases"].GetType()
					== json::ValueType::ArrayVal)) {
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

			double range[4];
			double *rng = NULL;
			double assoc[2];
			double * ass = NULL;
			std::string file = "";

			// get the phase name
			std::string phs = obj["PhaseName"].ToString();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using association phase: " + phs);

			// get the Range if present, otherwise look for an Assoc
			if (obj.HasKey("Range")
					&& (obj["Range"].GetType() == json::ValueType::ArrayVal)) {
				// get the range array
				json::Array arr = obj["Range"].ToArray();

				// make sure the range array has the correct number of entries
				if (arr.size() != 4) {
					continue;
				}

				// copy out the range values
				for (int i = 0; i < 4; i++) {
					range[i] = arr[i].ToDouble();
				}

				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Using association Range = ["
								+ std::to_string(range[0]) + ","
								+ std::to_string(range[1]) + ","
								+ std::to_string(range[2]) + ","
								+ std::to_string(range[3]) + "]");
				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Using association Assoc = ["
								+ std::to_string(assoc[0]) + ","
								+ std::to_string(assoc[1]) + "]");

				// set range pointer
				rng = range;

				// populate assoc from range
				assoc[0] = range[0];
				assoc[1] = range[3];

				// set assoc pointer
				ass = assoc;

			} else if (obj.HasKey("Assoc")
					&& (obj["Assoc"].GetType() == json::ValueType::ArrayVal)) {
				// get the assoc array
				json::Array arr = obj["Assoc"].ToArray();

				// make sure the assoc array has the correct number of entries
				if (arr.size() != 2) {
					continue;
				}

				// copy out the assoc values
				for (int i = 0; i < 2; i++) {
					assoc[i] = arr[i].ToDouble();
				}

				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Using association Assoc = ["
								+ std::to_string(assoc[0]) + ","
								+ std::to_string(assoc[1]) + "]");

				// set range pointer
				rng = NULL;

				// set assoc pointer
				ass = assoc;
			} else {
				glassutil::CLogit::log(
						glassutil::log_level::error,
						"CGlass::initialize: Missing required Range or Assoc key.");
				continue;
			}

			// get the travel time file if present
			if (obj.HasKey("TravFile")
					&& (obj["TravFile"].GetType() == json::ValueType::StringVal)) {
				file = obj["TravFile"].ToString();

				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Using association tt file: "
								+ file);
			} else {
				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Using default file location for association phase: "
								+ phs);
			}

			// set up this phase
			m_pAssociationTravelTimes->addPhase(phs, rng, ass, file);

			// test this phase
			if (m_bTestTravelTimes) {
				m_pAssociationTravelTimes->testTravelTimes(phs);
			}
		}
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
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
				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Plotting output is on!!!");
			}

			if (m_bGraphicsOut == false) {
				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Plotting output is off.");
			}
		}

		if ((paramsPlot.HasKey("graphicsStepKM"))
				&& (paramsPlot["graphicsStepKM"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dGraphicsStepKM = paramsPlot["graphicsStepKM"].ToDouble();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Plotting Step Increment: "
							+ std::to_string(m_dGraphicsStepKM));
		}

		if ((paramsPlot.HasKey("graphicsSteps"))
				&& (paramsPlot["graphicsSteps"].GetType()
						== json::ValueType::IntVal)) {
			m_iGraphicsSteps = paramsPlot["graphicsSteps"].ToInt();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Plotting Steps: "
							+ std::to_string(m_iGraphicsSteps));
		}

		if ((paramsPlot.HasKey("graphicsOutFolder"))
				&& (paramsPlot["graphicsOutFolder"].GetType()
						== json::ValueType::StringVal)) {
			m_sGraphicsOutFolder = paramsPlot["graphicsOutFolder"].ToString();
			glassutil::CLogit::log(
					glassutil::log_level::info,
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

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using NucleationStackThreshold: "
							+ std::to_string(m_dNucleationStackThreshold));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default NucleationStackThreshold: "
							+ std::to_string(m_dNucleationStackThreshold));
		}

		// Nucleate
		if ((params.HasKey("NucleationDataThreshold"))
				&& (params["NucleationDataThreshold"].GetType()
						== json::ValueType::IntVal)) {
			m_iNucleationDataThreshold =
					params["NucleationDataThreshold"].ToInt();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using NucleationDataThreshold: "
							+ std::to_string(m_iNucleationDataThreshold));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default NucleationDataThreshold: "
							+ std::to_string(m_iNucleationDataThreshold));
		}

		// sdAssociate
		if ((params.HasKey("AssociationStandardDeviationCutoff"))
				&& (params["AssociationStandardDeviationCutoff"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dAssociationSDCutoff =
					params["AssociationStandardDeviationCutoff"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using AssociationStandardDeviationCutoff: "
							+ std::to_string(m_dAssociationSDCutoff));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default AssociationStandardDeviationCutoff: "
							+ std::to_string(m_dAssociationSDCutoff));
		}

		// sdPrune
		if ((params.HasKey("PruningStandardDeviationCutoff"))
				&& (params["PruningStandardDeviationCutoff"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dPruningSDCutoff = params["PruningStandardDeviationCutoff"]
					.ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using PruningStandardDeviationCutoff: "
							+ std::to_string(m_dPruningSDCutoff));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default PruningStandardDeviationCutoff: "
							+ std::to_string(m_dPruningSDCutoff));
		}

		// ExpAffinity
		if ((params.HasKey("PickAffinityExponentialFactor"))
				&& (params["PickAffinityExponentialFactor"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dPickAffinityExpFactor = params["PickAffinityExponentialFactor"]
					.ToDouble();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using PickAffinityExponentialFactor: "
							+ std::to_string(m_dPickAffinityExpFactor));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default PickAffinityExponentialFactor: "
							+ std::to_string(m_dPickAffinityExpFactor));
		}

		// dCutFactor
		if ((params.HasKey("DistanceCutoffFactor"))
				&& (params["DistanceCutoffFactor"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dDistanceCutoffFactor = params["DistanceCutoffFactor"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using DistanceCutoffFactor: "
							+ std::to_string(m_dDistanceCutoffFactor));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default DistanceCutoffFactor: "
							+ std::to_string(m_dDistanceCutoffFactor));
		}

		// dCutPercentage
		if ((params.HasKey("DistanceCutoffPercentage"))
				&& (params["DistanceCutoffPercentage"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dDistanceCutoffPercentage = params["DistanceCutoffPercentage"]
					.ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using DistanceCutoffPercentage: "
							+ std::to_string(m_dDistanceCutoffPercentage));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default DistanceCutoffPercentage: "
							+ std::to_string(m_dDistanceCutoffPercentage));
		}

		// dCutMin
		if ((params.HasKey("DistanceCutoffMinimum"))
				&& (params["DistanceCutoffMinimum"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dMinDistanceCutoff = params["DistanceCutoffMinimum"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using DistanceCutoffMinimum: "
							+ std::to_string(m_dMinDistanceCutoff));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default DistanceCutoffMinimum: "
							+ std::to_string(m_dMinDistanceCutoff));
		}

		// iCycleLimit
		if ((params.HasKey("HypoProcessCountLimit"))
				&& (params["HypoProcessCountLimit"].GetType()
						== json::ValueType::IntVal)) {
			m_iProcessLimit = params["HypoProcessCountLimit"].ToInt();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using HypoProcessCountLimit: "
							+ std::to_string(m_iProcessLimit));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default HypoProcessCountLimit: "
							+ std::to_string(m_iProcessLimit));
		}

		// CorrelationTimeWindow
		if ((params.HasKey("CorrelationTimeWindow"))
				&& (params["CorrelationTimeWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dCorrelationMatchingTimeWindow = params["CorrelationTimeWindow"]
					.ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using CorrelationTimeWindow: "
							+ std::to_string(m_dCorrelationMatchingTimeWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
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

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using CorrelationDistanceWindow: "
							+ std::to_string(
									m_dCorrelationMatchingDistanceWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
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

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using CorrelationCancelAge: "
							+ std::to_string(m_iCorrelationCancelAge));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default CorrelationCancelAge: "
							+ std::to_string(m_iCorrelationCancelAge));
		}

		// HypocenterTimeWindow
		if ((params.HasKey("HypocenterTimeWindow"))
				&& (params["HypocenterTimeWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dHypoMergingTimeWindow =
					params["HypocenterTimeWindow"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using HypocenterTimeWindow: "
							+ std::to_string(m_dHypoMergingTimeWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
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

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using HypocenterDistanceWindow: "
							+ std::to_string(m_dHypoMergingDistanceWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
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

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using BeamMatchingAzimuthWindow: "
							+ std::to_string(m_dBeamMatchingAzimuthWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default "
							"BeamMatchingAzimuthWindow: "
							+ std::to_string(m_dBeamMatchingAzimuthWindow));
		}

		/*
		 * NOTE: Keep for when beams are fully implemented
		 // beamMatchingDistanceWindow
		 if ((params.HasKey("dBeamMatchingDistanceWindow"))
		 && (params["dBeamMatchingDistanceWindow"].GetType()
		 == json::ValueType::DoubleVal)) {
		 beamMatchingDistanceWindow = params["dBeamMatchingDistanceWindow"]
		 .ToDouble();

		 glassutil::CLogit::log(
		 glassutil::log_level::info,
		 "CGlass::initialize: Using dBeamMatchingDistanceWindow: "
		 + std::to_string(beamMatchingDistanceWindow));
		 } else {
		 glassutil::CLogit::log(
		 glassutil::log_level::info,
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

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using ReportingStackThreshold: "
							+ std::to_string(m_dReportingStackThreshold));
		} else {
			// default to overall thresh
			m_dReportingStackThreshold = m_dNucleationStackThreshold;
			glassutil::CLogit::log(
					glassutil::log_level::info,
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

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using ReportingDataThreshold: "
							+ std::to_string(m_iReportingDataThreshold));
		} else {
			// default to overall nNucleate
			m_iReportingDataThreshold = 0;
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default ReportingDataThreshold: "
							+ std::to_string(m_iReportingDataThreshold));
		}
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default association and nucleation "
				"parameters");
	}

	// Test Locator
	if ((com->HasKey("TestLocator"))
			&& ((*com)["TestLocator"].GetType() == json::ValueType::BoolVal)) {
		m_bTestLocator = (*com)["TestLocator"].ToBool();

		if (m_bTestLocator) {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: testLocator set to true");
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: testLocator set to false");
		}
	} else {
		glassutil::CLogit::log(glassutil::log_level::info,
								"CGlass::initialize: testLocator not Found!");
	}

	// set maximum number of picks
	if ((com->HasKey("MaximumNumberOfPicks"))
			&& ((*com)["MaximumNumberOfPicks"].GetType()
					== json::ValueType::IntVal)) {
		m_iMaxNumPicks = (*com)["MaximumNumberOfPicks"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using MaximumNumberOfPicks: "
						+ std::to_string(m_iMaxNumPicks));
	}

	// set maximum number of pick in a single site
	if ((com->HasKey("MaximumNumberOfPicksPerSite"))
			&& ((*com)["MaximumNumberOfPicksPerSite"].GetType()
					== json::ValueType::IntVal)) {
		m_iMaxNumPicksPerSite = (*com)["MaximumNumberOfPicksPerSite"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using MaximumNumberOfPicksPerSite: "
						+ std::to_string(m_iMaxNumPicksPerSite));
	}

	// set maximum number of correlations
	if ((com->HasKey("MaximumNumberOfCorrelations"))
			&& ((*com)["MaximumNumberOfCorrelations"].GetType()
					== json::ValueType::IntVal)) {
		m_iMaxNumCorrelations = (*com)["MaximumNumberOfCorrelations"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using MaximumNumberOfCorrelations: "
						+ std::to_string(m_iMaxNumCorrelations));
	}

	// set maximum number of hypos
	if ((com->HasKey("MaximumNumberOfHypos"))
			&& ((*com)["MaximumNumberOfHypos"].GetType()
					== json::ValueType::IntVal)) {
		m_iMaxNumHypos = (*com)["MaximumNumberOfHypos"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using MaximumNumberOfHypos: "
						+ std::to_string(m_iMaxNumHypos));
	}

	// set pick duplicate window
	if ((com->HasKey("PickDuplicateWindow"))
			&& ((*com)["PickDuplicateWindow"].GetType()
					== json::ValueType::DoubleVal)) {
		m_dPickDuplicateTimeWindow = (*com)["PickDuplicateWindow"].ToDouble();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using PickDuplicateWindow: "
						+ std::to_string(m_dPickDuplicateTimeWindow));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default PickDuplicateWindow: "
						+ std::to_string(m_dPickDuplicateTimeWindow));
	}



	// set the number of nucleation threads
	int numNucleationThreads = 5;
	if ((com->HasKey("NumberOfNucleationThreads"))
			&& ((*com)["NumberOfNucleationThreads"].GetType()
					== json::ValueType::IntVal)) {
		numNucleationThreads = (*com)["NumberOfNucleationThreads"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using NumberOfNucleationThreads: "
						+ std::to_string(numNucleationThreads));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default NumberOfNucleationThreads: "
						+ std::to_string(numNucleationThreads));
	}

	// set the number of hypo threads
	int numHypoThreads = 3;
	if ((com->HasKey("NumberOfHypoThreads"))
			&& ((*com)["NumberOfHypoThreads"].GetType()
					== json::ValueType::IntVal)) {
		numHypoThreads = (*com)["NumberOfHypoThreads"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using NumberOfHypoThreads: "
						+ std::to_string(numHypoThreads));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default NumberOfHypoThreads: "
						+ std::to_string(numHypoThreads));
	}

	// set the number of web threads
	int numWebThreads = 0;
	if ((com->HasKey("NumberOfWebThreads"))
			&& ((*com)["NumberOfWebThreads"].GetType()
					== json::ValueType::IntVal)) {
		numWebThreads = (*com)["NumberOfWebThreads"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using NumberOfWebThreads: "
						+ std::to_string(numWebThreads));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default NumberOfWebThreads: "
						+ std::to_string(numWebThreads));
	}

	int iHoursWithoutPicking = -1;
	if ((com->HasKey("SiteHoursWithoutPicking"))
			&& ((*com)["SiteHoursWithoutPicking"].GetType()
					== json::ValueType::IntVal)) {
		iHoursWithoutPicking = (*com)["SiteHoursWithoutPicking"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using SiteHoursWithoutPicking: "
						+ std::to_string(iHoursWithoutPicking));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default SiteHoursWithoutPicking: "
						+ std::to_string(iHoursWithoutPicking));
	}

	int iHoursBeforeLookingUp = -1;
	if ((com->HasKey("SiteLookupInterval"))
			&& ((*com)["SiteLookupInterval"].GetType()
					== json::ValueType::IntVal)) {
		iHoursBeforeLookingUp = (*com)["SiteLookupInterval"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using SiteLookupInterval: "
						+ std::to_string(iHoursBeforeLookingUp));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default SiteLookupInterval: "
						+ std::to_string(iHoursBeforeLookingUp));
	}

	int iMaxPicksPerHour = -1;
	if ((com->HasKey("SiteMaximumPicksPerHour"))
			&& ((*com)["SiteMaximumPicksPerHour"].GetType()
					== json::ValueType::IntVal)) {
		iMaxPicksPerHour = (*com)["SiteMaximumPicksPerHour"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using SiteMaximumPicksPerHour: "
						+ std::to_string(iMaxPicksPerHour));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default SiteMaximumPicksPerHour: "
						+ std::to_string(iMaxPicksPerHour));
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
double CGlass::getDistanceCutoffPercentage() {
	return (m_dDistanceCutoffPercentage);
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

// ------------------------------------------------getNumStationsPerNode
int CGlass::getNumStationsPerNode() {
	return (m_iNumStationsPerNode);
}

// ------------------------------------------------getNucleationDataThreshold
int CGlass::getNucleationDataThreshold() {
	return (m_iNucleationDataThreshold);
}

// ------------------------------------------------getReportingDataThreshold
double CGlass::getReportingDataThreshold() {
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

// ------------------------------------------------getMaxNumPicksPerSite
int CGlass::getMaxNumPicksPerSite() {
	return (m_iMaxNumPicksPerSite);
}

// ------------------------------------------------setMaxNumPicksPerSite
void CGlass::setMaxNumPicksPerSite(int max) {
	m_iMaxNumPicksPerSite = max;
}

// ------------------------------------------------getMaxNumHypos
int CGlass::getMaxNumHypos() {
	return (m_iMaxNumHypos);
}

// ------------------------------------------------setMaxNumHypos
void CGlass::setMaxNumHypos(int max) {
	m_iMaxNumHypos = max;
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

/* NOTE: Leave these in place as examples for Travel Time unit tests.
 *
 // ---------------------------------------------------------Test
 // General testing wart
 bool CGlass::test(json::Object *com) {
 CTerra *terra = new CTerra();
 CRay *ray = new CRay();
 double deg2rad = 0.01745329251994;
 double rad2deg = 57.29577951308;
 ray->pTerra = terra;
 string mdl = (*com)["Model"].ToString();
 printf("model:%s\n", mdl.c_str());

 bool bload = terra->load(mdl.c_str());
 if (!bload)
 return(false;
 printf("Terra nLayer:%ld dRadius:%.2f\n", terra->nLayer, terra->dRadius);
 double r = terra->dRadius;

 int iphs = ray->setPhase("P");
 printf("ray->setPhase:%d\n", iphs);
 ray->setDepth(100.0);
 ray->Setup();
 double t1;
 double t2;
 double dtdz;
 double rad;
 for (double deg = 0.0; deg < 30.0; deg += 1.0) {
 rad = deg2rad * deg;
 t1 = ray->Travel(rad, r);
 t2 = ray->Travel(rad, r + 1.0);
 printf("T %6.2f %6.2f %6.2f %6.2f\n", deg, t1, t2, t2 - t1);
 }

 return(true;
 }

 // ---------------------------------------------------------GenTrv
 // Generate travel time interpolation file
 bool CGlass::genTrv(json::Object *com) {
 CGenTrv *gentrv;

 gentrv = new CGenTrv();
 bool bres = gentrv->Generate(com);
 delete gentrv;
 return(bres;
 }

 // ---------------------------------------------------------TestTTT
 // Quick and dirty unit test for TTT functions
 void CGlass::testTTT(json::Object *com) {
 pTerra = new CTerra();
 pRay = new CRay();
 double deg2rad = 0.01745329251994;
 double rad2deg = 57.29577951308;
 pRay->pTerra = pTerra;
 string mdl = (*com)["Model"].ToString();
 printf("model:%s\n", mdl.c_str());
 bool bload = pTerra->load(mdl.c_str());
 if (!bload)
 printf(" ** ERR:Cannot load Earth model\n");
 printf("Terra nLayer:%ld dRadius:%.2f\n", pTerra->nLayer, pTerra->dRadius);
 CTravelTime *trv = new CTravelTime();
 trv->Setup(pRay, "PKPdf");
 trv->setOrigin(0.0, 0.0, 50.0);
 double t = trv->T(50.0);
 printf("T(50.0,50.0) is %.2f\n", t);
 int mn;
 double sc;
 trv->setOrigin(0.0, 0.0, 0.0);
 for (double d = 0.0; d < 360.5; d += 1.0) {
 t = trv->T(d);
 mn = (int) (t / 60.0);
 sc = t - 60.0 * mn;
 printf("%6.1f %3d %5.2f\n", d, mn, sc);
 }
 CTTT *ttt = new CTTT();
 ttt->Setup(pRay);
 ttt->addPhase("P");
 ttt->addPhase("S");
 ttt->setOrigin(0.0, 0.0, 0.0);
 for (double d = 0.0; d < 360.5; d += 1.0) {
 t = ttt->T(d, "P");
 mn = (int) (t / 60.0);
 sc = t - 60.0 * mn;
 printf("%6.1f %3d %5.2f\n", d, mn, sc);
 }
 }*/

}  // namespace glasscore
