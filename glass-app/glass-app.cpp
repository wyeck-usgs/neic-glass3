// glass-app.cpp : Defines the entry point for the console application.
#include <project_version.h>
#include <json.h>
#include <logger.h>
#include <config.h>
#include <file_input.h>
#include <file_output.h>
#include <associator.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <memory>

int main(int argc, char* argv[]) {
	std::string configdir = "";

	// check our arguments
	if ((argc < 2) || (argc > 3)) {
		std::cout << "glass-app version "
					<< std::to_string(PROJECT_VERSION_MAJOR) << "."
					<< std::to_string(PROJECT_VERSION_MINOR) << "."
					<< std::to_string(PROJECT_VERSION_PATCH) << "; Usage: "
					<< "glass-app <configfile> [noconsole]" << std::endl;
		return 1;
	}

	// Look up our log directory
	std::string logpath;
	char* pLogDir = getenv("GLASS_LOG");
	if (pLogDir != NULL) {
		logpath = pLogDir;
	} else {
		logpath = "./";
	}

	// get whether we log to console
	bool logConsole = true;
	if (argc == 3) {
		std::string temp = std::string(argv[2]);

		if (temp == "noconsole") {
			logConsole = false;
		}
	}

	// now set up our logging
	glass3::util::log_init("glass-app", spdlog::level::info, logpath, logConsole);

	glass3::util::log(
			"info",
			"glass-app: neic-glass3 Version "
					+ std::to_string(PROJECT_VERSION_MAJOR) + "."
					+ std::to_string(PROJECT_VERSION_MINOR) + "."
					+ std::to_string(PROJECT_VERSION_PATCH) + " startup.");

	glass3::util::log(
			"info",
			"glass-app: loading configuration file " + std::string(argv[1]));

	// now load our basic config from file
	// note, main is gonna get cluttered, move this section
	// to it's own function eventually
	glass3::util::Config * glassConfig = new glass3::util::Config();

	try {
		glassConfig->parseJSONFromFile("", std::string(argv[1]));
	} catch (std::exception& e) {
		glass3::util::log(
				"critcalerror",
				"Failed to load file: " + std::string(argv[1]) + "; "
						+ std::string(e.what()));

		delete (glassConfig);
		return (1);
	}

	// check to see if our json config is of the right format
	if (!(glassConfig->getJSON()->HasKey("Cmd"))) {
		glass3::util::log("critcalerror", "Invalid configuration, exiting.");

		delete (glassConfig);
		return (1);
	}
	if ((*glassConfig->getJSON())["Cmd"] != "Glass") {
		glass3::util::log("critcalerror", "Wrong configuration, exiting.");

		delete (glassConfig);
		return (1);
	}

	// get the directory where the rest of the glass configs are stored
	if (!(glassConfig->getJSON()->HasKey("ConfigDirectory"))) {
		configdir = "./";
		glass3::util::log(
				"warning",
				"missing <ConfigDirectory>, defaulting to local directory.");
	} else {
		configdir = (*glassConfig->getJSON())["ConfigDirectory"].ToString();
		glass3::util::log("info", "Reading glass configurations from: " + configdir);
	}

	// set our proper loglevel
	if (glassConfig->getJSON()->HasKey("LogLevel")) {
		glass3::util::log_update_level((*glassConfig->getJSON())["LogLevel"]);
	}

	// load our initialize config
	std::string initconfigfile;
	if (!(glassConfig->getJSON()->HasKey("InitializeFile"))) {
		glass3::util::log(
				"critcalerror",
				"Invalid configuration, missing <InitializeFile>, exiting.");

		delete (glassConfig);
		return (1);
	} else {
		initconfigfile = (*glassConfig->getJSON())["InitializeFile"].ToString();
	}

	glass3::util::Config * InitializeConfig;
	try {
		InitializeConfig = new glass3::util::Config(configdir, initconfigfile);
	} catch (std::exception& e) {
		glass3::util::log(
				"critcalerror",
				"Failed to load file: " + initconfigfile + "; "
						+ std::string(e.what()));

		delete (glassConfig);
		delete (InitializeConfig);
		return (1);
	}

	std::shared_ptr<const json::Object> InitializeJSON = InitializeConfig
			->getJSON();

	// Load our initial stationlist
	std::string stationlistfile;
	if (!(glassConfig->getJSON()->HasKey("StationList"))) {
		glass3::util::log("critcalerror",
					"Invalid configuration, missing <StationList>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		return (1);
	} else {
		stationlistfile = (*glassConfig->getJSON())["StationList"].ToString();
	}

	// get our stationlist
	glass3::util::Config * StationList;
	try {
		StationList = new glass3::util::Config(configdir, stationlistfile);
	} catch (std::exception& e) {
		glass3::util::log(
				"critcalerror",
				"Failed to load file: " + stationlistfile + "; "
						+ std::string(e.what()));

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		return (1);
	}

	std::shared_ptr<const json::Object> StationListJSON =
			StationList->getJSON();

	// get detection grid file list
	json::Array gridconfigfilelist;
	if (glassConfig->getJSON()->HasKey("GridFiles")
			&& ((*glassConfig->getJSON())["GridFiles"].GetType()
					== json::ValueType::ArrayVal)) {
		gridconfigfilelist = (*glassConfig->getJSON())["GridFiles"];
	} else {
		glass3::util::log("critcal",
					"Invalid configuration, missing <GridFiles>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		return (1);
	}

	// check to see if any files were in the array
	if (gridconfigfilelist.size() == 0) {
		glass3::util::log("critcal", "No <GridFiles> specified, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		return (1);
	}

	// load our input config
	std::string inputconfigfile;
	if (!(glassConfig->getJSON()->HasKey("InputConfig"))) {
		glass3::util::log("critcalerror",
					"Invalid configuration, missing <InputConfig>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		return (1);
	} else {
		inputconfigfile = (*glassConfig->getJSON())["InputConfig"].ToString();
	}

	glass3::util::Config * InputConfig;
	try {
		InputConfig = new glass3::util::Config(configdir, inputconfigfile);
	} catch (std::exception& e) {
		glass3::util::log(
				"critcalerror",
				"Failed to load file: " + inputconfigfile + "; "
						+ std::string(e.what()));

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		delete (InputConfig);
		return (1);
	}

	// load our output config
	std::string outputconfigfile;
	if (!(glassConfig->getJSON()->HasKey("OutputConfig"))) {
		glass3::util::log("critcalerror",
					"Invalid configuration, missing <OutputConfig>, exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		delete (InputConfig);
		return (1);
	} else {
		outputconfigfile = (*glassConfig->getJSON())["OutputConfig"].ToString();
	}
	glass3::util::Config * OutputConfig;
	try {
		OutputConfig = new glass3::util::Config(configdir, outputconfigfile);
	} catch (std::exception& e) {
		glass3::util::log(
				"critcalerror",
				"Failed to load file: " + outputconfigfile + "; "
						+ std::string(e.what()));

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		delete (InputConfig);
		delete (OutputConfig);
		return (1);
	}

	// create our objects
	glass::fileInput * InputThread = new glass::fileInput();
	glass::fileOutput * OutputThread = new glass::fileOutput();
	glass::Associator * AssocThread = new glass::Associator();

	// input setup
	std::shared_ptr<const json::Object> input_config_json =
			InputConfig->getJSON();
	if (InputThread->setup(input_config_json) != true) {
		glass3::util::log("critical", "glass: Failed to setup Input.  Exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		delete (InputConfig);
		delete (OutputConfig);
		delete (InputThread);
		delete (OutputThread);
		delete (AssocThread);
		return (1);
	}

	// output setup
	std::shared_ptr<const json::Object> output_config_json = OutputConfig
			->getJSON();
	if (OutputThread->setup(output_config_json) != true) {
		glass3::util::log("critical", "glass: Failed to setup Output.  Exiting.");

		delete (glassConfig);
		delete (InitializeConfig);
		delete (StationList);
		delete (InputConfig);
		delete (OutputConfig);
		delete (InputThread);
		delete (OutputThread);
		delete (AssocThread);
		return (1);
	}

	// output needs to know about the associator thread to request
	// information
	OutputThread->setAssociator(AssocThread);

	// assoc thread needs to know about the input and output threads
	AssocThread->Input = InputThread;
	AssocThread->Output = OutputThread;

	// configure glass
	// first send in initialize
	AssocThread->setup(InitializeJSON);

	// send in stationlist
	AssocThread->setup(StationListJSON);

	// send in grids
	for (int i = 0; i < gridconfigfilelist.size(); i++) {
		std::string gridconfigfile = gridconfigfilelist[i];
		if (gridconfigfile != "") {
			glass3::util::Config * GridConfig = new glass3::util::Config(
					configdir, gridconfigfile);
			std::shared_ptr<const json::Object> GridConfigJSON = GridConfig
					->getJSON();

			// send in grid
			AssocThread->setup(GridConfigJSON);

			// done with grid config and json
			delete (GridConfig);
		}
	}

	// startup
	InputThread->start();
	OutputThread->start();
	AssocThread->start();

	glass3::util::log("info", "glass: glass is running.");

	// run until stopped
	while (true) {
		// sleep to give up cycles
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));

		glass3::util::log("trace", "glass: Checking thread status.");

		// check thread health
		if (InputThread->healthCheck() == false) {
			glass3::util::log("error", "glass: Input thread has exited!!");
			break;
		} else if (OutputThread->healthCheck() == false) {
			glass3::util::log("error", "glass: Output thread has exited!!");
			break;
		} else if (AssocThread->healthCheck() == false) {
			glass3::util::log("error", "glass: Association thread has exited!!");
			break;
		}
	}

	glass3::util::log("info", "glass: glass is shutting down.");

	// shutdown
	InputThread->stop();
	OutputThread->stop();
	AssocThread->stop();

	// cleanup
	delete (glassConfig);
	delete (InitializeConfig);
	delete (StationList);
	delete (InputConfig);
	delete (OutputConfig);
	delete (InputThread);
	delete (OutputThread);
	delete (AssocThread);

	return 0;
}
