#include "ros/ros.h"
#include "visualization_msgs/Marker.h"

#include "knowledge_msgs/PlanningCmdClips.h"
#include "knowledge_msgs/planning_cmd.h"
#include "knowledge_msgs/StrQueryKDB.h"

#include "justina_tools/JustinaHardware.h"
#include "justina_tools/JustinaHRI.h"
#include "justina_tools/JustinaManip.h"
#include "justina_tools/JustinaNavigation.h"
#include "justina_tools/JustinaTools.h"
#include "justina_tools/JustinaVision.h"
#include "justina_tools/JustinaTasks.h"
#include "justina_tools/JustinaManip.h"
#include "justina_tools/JustinaRepresentation.h"

#include <vector>
#include <ctime>
#include <map>

using namespace boost::algorithm;

enum SMState {
	SM_INIT,
	SM_SAY_WAIT_FOR_DOOR,
	SM_WAIT_FOR_DOOR,
	SM_NAVIGATE_TO_THE_LOCATION,
	SM_SEND_INIT_CLIPS,
	SM_RUN_SM_CLIPS
};

ros::Publisher command_response_pub;
std::string testPrompt;
SMState state = SM_INIT;
bool runSMCLIPS = false;
bool startSignalSM = false;
knowledge_msgs::PlanningCmdClips initMsg;

// This is for the attemps for a actions
std::string lastCmdName = "";
std::string currentName = "";
std::string objectName = "";
std::string categoryName = "";
int numberAttemps = 0;
int cantidad = 0;
int women;
int men;
int sitting;
int standing;
int lying;
ros::Time beginPlan;
bool fplan = false;
double maxTime = 180;
std::string cat_grammar= "gpsrII_nagoya.xml";

ros::ServiceClient srvCltGetTasks;
ros::ServiceClient srvCltInterpreter;
ros::ServiceClient srvCltWaitConfirmation;
ros::ServiceClient srvCltWaitForCommand;
ros::ServiceClient srvCltAnswer;
ros::ServiceClient srvCltAskName;
ros::ServiceClient srvCltAskIncomplete;
ros::ServiceClient srvCltQueryKDB;

void validateAttempsResponse(knowledge_msgs::PlanningCmdClips msg) {
	//lastCmdName = msg.name;
	if (msg.successful == 0
			&& (msg.name.compare("move_actuator") == 0
					|| msg.name.compare("find_object") == 0
					|| msg.name.compare("status_object") == 0
					|| msg.name.compare("many_obj") == 0
					|| msg.name.compare("answer") == 0
					|| msg.name.compare("drop") == 0)) {
		if (msg.name.compare(lastCmdName) != 0)
			numberAttemps = 0;
		else if (numberAttemps == 0) {
			msg.successful = 1;
			numberAttemps = 0;
		} else
			numberAttemps++;
	}
	else if (msg.successful == 1){
		numberAttemps = 0;
	}
	
	lastCmdName = msg.name;
	command_response_pub.publish(msg);
}

bool validateContinuePlan(double currentTime, bool fplan)
{
	bool result = true;

	if (currentTime >= maxTime && fplan){
		std::stringstream ss;
		knowledge_msgs::StrQueryKDB srv;
		ss.str("");
		ss << "(assert (cmd_finish_plan 1))";
		srv.request.query = ss.str();
		if (srvCltQueryKDB.call(srv)) {
			std::cout << "Response of KBD Query:" << std::endl;
			std::cout << "TEST QUERY Args:" << srv.response.result << std::endl;
			result = false;
			beginPlan = ros::Time::now();
		} else {
			std::cout << testPrompt << "Failed to call service of KBD query"<< std::endl;
			result =  true;
		}
	}
	
	return result;

}

void callbackCmdSpeech(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command Speech ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;
	bool success = true;
	startSignalSM = true;

	if (!runSMCLIPS)
		success = false;

	success = success
			& ros::service::waitForService("/planning_clips/wait_command",
					50000);
	if (success) {
		knowledge_msgs::planning_cmd srv;
		srv.request.name = "test_wait";
		srv.request.params = "Ready";
		if (srvCltWaitForCommand.call(srv)) {
			std::cout << "Response of wait for command:" << std::endl;
			std::cout << "Success:" << (long int) srv.response.success
					<< std::endl;
			std::cout << "Args:" << srv.response.args << std::endl;
		} else {
			std::cout << testPrompt << "Failed to call service of wait_command"
					<< std::endl;
			responseMsg.successful = 0;
		}
		responseMsg.params = srv.response.args;
		responseMsg.successful = srv.response.success;
	} else {
		if (!runSMCLIPS) {
			initMsg = responseMsg;
			return;
		}
		std::cout << testPrompt << "Needed services are not available :'("
				<< std::endl;
		responseMsg.successful = 0;
	}
	if (runSMCLIPS) {
		validateAttempsResponse(responseMsg);
		//command_response_pub.publish(responseMsg);
	}
}

void callbackCmdInterpret(
		const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command interpreter ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	bool success = ros::service::waitForService("/planning_clips/spr_interpreter",
			5000);
	if (success) {
		knowledge_msgs::planning_cmd srv;
		srv.request.name = "test_interprete";
		srv.request.params = "Ready to interpretation";
		if (srvCltInterpreter.call(srv)) {
			std::cout << "Response of interpreter:" << std::endl;
			std::cout << "Success:" << (long int) srv.response.success
					<< std::endl;
			std::cout << "Args:" << srv.response.args << std::endl;
			responseMsg.params = srv.response.args;
			responseMsg.successful = srv.response.success;
		} else {
			std::cout << testPrompt << "Failed to call service of interpreter"
					<< std::endl;
			responseMsg.successful = 0;
		}
	} else {
		std::cout << testPrompt << "Needed services are not available :'("
				<< std::endl;
		responseMsg.successful = 0;
	}
	validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);

}

void callbackCmdConfirmation(
		const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command confirmation ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	bool success = ros::service::waitForService("spg_say", 5000);
	success = success & ros::service::waitForService("/planning_clips/confirmation",5000);
	if (success) {
		std::string to_spech = responseMsg.params;
		boost::replace_all(to_spech, "_", " ");
		std::stringstream ss;
		ss << "Do you want me " << to_spech;
		std::cout << "------------- to_spech: ------------------ " << ss.str()
				<< std::endl;

		JustinaHRI::waitAfterSay(ss.str(), 2500);

		knowledge_msgs::planning_cmd srv;
		srv.request.name = "test_confirmation";
		srv.request.params = responseMsg.params;
		if (srvCltWaitConfirmation.call(srv)) {
			std::cout << "Response of confirmation:" << std::endl;
			std::cout << "Success:" << (long int) srv.response.success
					<< std::endl;
			std::cout << "Args:" << srv.response.args << std::endl;
			if (srv.response.success){
				JustinaHRI::waitAfterSay("Ok i start to execute the command", 2000);
                /*float currx, curry, currtheta;
                JustinaKnowledge::addUpdateKnownLoc("current_loc", currx, curry);*/
				beginPlan = ros::Time::now();
			}
			else
				JustinaHRI::waitAfterSay("Repeate the command please", 2000);

			responseMsg.params = srv.response.args;
			responseMsg.successful = srv.response.success;
		} else {
			std::cout << testPrompt << "Failed to call service of confirmation"
					<< std::endl;
			responseMsg.successful = 0;
			JustinaHRI::waitAfterSay("Repeate the command please", 2000);
		}
	} else {
		std::cout << testPrompt << "Needed services are not available :'("
				<< std::endl;
		responseMsg.successful = 0;
	}
	validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);
}

void callbackCmdSpeechGenerator(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg){
	std::cout << testPrompt << "--------- Command Speech Generator-----" << std::endl;
	std::cout << "name: " << msg->name << std::endl;
	std::cout << "params: " << msg->params << std::endl;
	
	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::stringstream ss;
        std::vector<std::string> tokens;
        std::string str = responseMsg.params;
        split(tokens, str, is_any_of("_"));
	
	
	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;

	ss << tokens[0];
	for(int i=1 ; i<tokens.size(); i++)
		ss << " "<< tokens[i];
	
	JustinaHRI::waitAfterSay(ss.str(), 10000);
	
	responseMsg.successful = 1;

	command_response_pub.publish(responseMsg);
	

}

void callbackCmdGetTasks(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command get tasks ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	bool success = ros::service::waitForService("/planning_clips/get_task",
			5000);
	if (success) {
		knowledge_msgs::planning_cmd srv;
		srv.request.name = "cmd_task";
		srv.request.params = "Test of get_task module";
		if (srvCltGetTasks.call(srv)) {
			std::cout << "Response of get tasks:" << std::endl;
			std::cout << "Success:" << (long int) srv.response.success
					<< std::endl;
			std::cout << "Args:" << srv.response.args << std::endl;
			responseMsg.params = srv.response.args;
			responseMsg.successful = srv.response.success;
		} else {
			std::cout << testPrompt << "Failed to call get tasks" << std::endl;
			responseMsg.successful = 0;
		}
	} else {
		std::cout << testPrompt << "Needed services are not available :'("
				<< std::endl;
		responseMsg.successful = 0;
	}
	validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);
}

void callbackCmdNavigation(
		const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command Navigation ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;

	bool success = true;
	bool nfp = true;	
	if (tokens[1] != "arena" && tokens[1] != "exitdoor")
		nfp = validateContinuePlan(d.toSec(), fplan);

	if (tokens[1] == "person") {
		success = true;
	} else {
		if (nfp)
			success = JustinaTasks::sayAndSyncNavigateToLoc(tokens[1], 120000);
	}
	if (success)
		responseMsg.successful = 1;
	else
		responseMsg.successful = 0;
	if (nfp)
		validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);
}

void callbackCmdAnswer(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {

	std::cout << testPrompt << "--------- Command answer a question ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::stringstream ss;
	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));

	std::map<int,std::string> weekdays;
	std::map<int,std::string> months;
	std::map<int,std::string> days;

  	weekdays[0]="sunday";
  	weekdays[1]="monday";
  	weekdays[2]="tuesday";
	weekdays[3]="wednesday";
	weekdays[4]="thursday";
	weekdays[5]="friday";
	weekdays[6]="saturday";

	months[0] = "january";
	months[1] = "february";
	months[2] = "march";
	months[3] = "april";
	months[4] = "may";
	months[5] = "june";
	months[6] = "july";
	months[7] = "august";
	months[8] = "september";
	months[9] = "october";
	months[10] = "november";
	months[11] = "december";

	days[1] = "first";
	days[2] = "second";
	days[3] = "third";
	days[4] = "fourth";
	days[5] = "fifth";
	days[6] = "sixth";
	days[7] = "seventh";
	days[8] = "eighth";
	days[9] = "ninth";
	days[10] = "tenth";
	days[11] = "eleventh";
	days[12] = "twelfth";
	days[13] = "thirteenth";
	days[14] = "fourteenth";
	days[15] = "fifteenth";
	days[16] = "sixteenth";
	days[17] = "seventeenth";
	days[18] = "eighteenth";
	days[19] = "nineteenth";
	days[20] = "twentieht";
	days[21] = "twenty first";
	days[22] = "twenty second";
	days[23] = "twenty third";
	days[24] = "twenty fourth";
	days[25] = "twenty fifth";
	days[26] = "twenty sixth";
	days[27] = "twenty seventh";
	days[28] = "twenty eighth";
	days[29] = "twenty ninth";
	days[30] = "thirtieth";
	days[31] = "thirty first";
	
	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;

	bool success = ros::service::waitForService("spg_say", 5000);
	if (success) {
		std::string param1 = tokens[1];
		if (param1.compare("a_question") == 0) {
			success = ros::service::waitForService("/planning_clips/answer",
					5000);
			JustinaHRI::loadGrammarSpeechRecognized("questions.xml");
			if (success) {
				success = JustinaHRI::waitAfterSay(
						"I am waiting for the user question", 2000);
				knowledge_msgs::planning_cmd srv;
				srvCltAnswer.call(srv);
				if (srv.response.success){
					success = JustinaHRI::waitAfterSay(srv.response.args, 2000);
					responseMsg.successful = 1;
				}
				else
					responseMsg.successful = 0;
			}
		} else if (param1.compare("your_name") == 0) {
			JustinaHRI::waitAfterSay("Hellow my name is justina", 2000);
			responseMsg.successful = 1;
		}else if (param1.compare("your_team_affiliation") == 0 || param1.compare("affiliation") == 0) {
			JustinaHRI::waitAfterSay("my team affiliation is the national autonomous university of mexico", 2000);
			responseMsg.successful = 1;
		}else if (param1.compare("your_team_country") == 0 || param1.compare("country") == 0) {
			JustinaHRI::waitAfterSay("My teams country is Mexico", 2000);
			responseMsg.successful = 1;
		} else if (param1.compare("your_team_name") == 0
				|| param1.compare("the_name_of_your_team") == 0 || param1.compare("name") == 0)  {
			JustinaHRI::waitAfterSay("Hello my team is pumas", 2000);
			responseMsg.successful = 1;
		} else if (param1.compare("introduce_yourself") == 0 || param1.compare("something_about_yourself") == 0) {
			JustinaHRI::waitAfterSay("I am going to introduce myself", 2000);
			JustinaHRI::waitAfterSay("My name is justina", 2000);
			JustinaHRI::waitAfterSay("i am from Mexico city", 2000);
			JustinaHRI::waitAfterSay("my team is pumas", 2000);
			JustinaHRI::waitAfterSay(
					"of the national autonomous university of mexico", 2000);
			responseMsg.successful = 1;
		} else if (param1.compare("the_day") == 0
				|| param1.compare("the_time") == 0) {
			ss.str("");
			//std::locale::global(std::locale("de_DE.utf8"));
			//std::locale::global(std::locale("en_us.utf8"));
			time_t now = time(0);
			char* dt = ctime(&now);
			std::cout << "Day:" << dt << std::endl;
			JustinaHRI::waitAfterSay(dt, 2000);
			responseMsg.successful = 1;
		} else if (param1.compare("what_time_is_it") == 0) {
			ss.str("");
			std::time_t now = time(0);
			std::tm *ltm = localtime(&now);
			ss << "The time is " << ltm->tm_hour << " hours" << ltm->tm_min << " minutes";
			JustinaHRI::waitAfterSay(ss.str(), 2000);
			responseMsg.successful = 1;
		} else if (param1.compare("what_day_is_tomorrow") == 0) {
			std::time_t now = time(0);
			std::tm *ltmnow = localtime(&now);
			std::cout << "Curr day :" << ltmnow->tm_mday << std::endl;
			ltmnow->tm_mday = ltmnow->tm_mday + 1;
			std::cout << "Curr month :" << ltmnow->tm_mon << std::endl;
			std::cout << "The day of month:" << ltmnow->tm_mday << std::endl;
			ss << "Tomorrow is " << months[ltmnow->tm_mon] << " " << days[ltmnow->tm_mday];
			JustinaHRI::waitAfterSay(ss.str(), 2000);
			responseMsg.successful = 1;
		}else if (param1.compare("the_day_of_the_week") == 0){
			ss.str("");
			std::time_t now = time(0);
			std::tm *ltmnow = localtime(&now);
			std::cout << "Curr day :" << ltmnow->tm_wday << std::endl;
			std::cout << "The day of week:" << ltmnow->tm_wday << std::endl;
			std::time_t day_week = std::mktime(ltmnow);
			std::cout << "Week day format :" << ltmnow->tm_wday << std::endl;
			ss << "The day of the week is " << weekdays[ltmnow->tm_wday];
			JustinaHRI::waitAfterSay(ss.str(), 2000);
			responseMsg.successful = 1;
		}else if (param1.compare("the_day_of_the_month") == 0 || param1.compare("what_day_is_today") == 0) {
			ss.str("");
			std::time_t now = time(0);
			std::tm *ltmnow = localtime(&now);
			std::cout << "Curr month :" << ltmnow->tm_mon << std::endl;
			std::cout << "The day of month:" << ltmnow->tm_mday << std::endl;
			ss << "Today is " << months[ltmnow->tm_mon] << " " << days[ltmnow->tm_mday];
			JustinaHRI::waitAfterSay(ss.str(), 2000);
			responseMsg.successful = 1;
		}else if (param1.compare("a_joke") == 0) {
			ss.str("");
			JustinaHRI::waitAfterSay("I am going to say a joke", 2000);
			JustinaHRI::waitAfterSay("What is the longest word in the English language", 2000);
			JustinaHRI::waitAfterSay("SMILES, there is a mile between the first and last letters", 2000);
			JustinaHRI::waitAfterSay("hee hee hee", 2000);
			responseMsg.successful = 1;
		}
		else if(param1.compare("tell_many_obj") == 0){
			ss.str("");
			ss << "I found " << cantidad << " "<< currentName;
			std::cout << ss.str() << std::endl;
			JustinaHRI::waitAfterSay(ss.str(), 2000);
			responseMsg.successful = 1;
		}
		else if(param1.compare("tell_what") == 0){
			ss.str("");
			if(objectName == "none"){
				ss << "I did not find objects";
				JustinaHRI::waitAfterSay(ss.str(), 2000);
				responseMsg.successful = 1;
			}
			else{
				ss << "The " << objectName << " is the " << currentName << " object I found"; 
				std::cout << ss.str() << std::endl;
				JustinaHRI::waitAfterSay(ss.str(), 2000);
				responseMsg.successful = 1;
			}
		}
		else if(param1.compare("tell_what_cat") == 0){
			ss.str("");
			if(objectName == "none"){
				ss << "I did not find the " << categoryName;
				JustinaHRI::waitAfterSay(ss.str(), 2000);
				responseMsg.successful = 1;
			}
			else{
				ss << "The " << objectName << " is the " << currentName << " " << categoryName <<" I found"; 
				std::cout << ss.str() << std::endl;
				JustinaHRI::waitAfterSay(ss.str(), 2000);
				responseMsg.successful = 1;
			}
		}
		else if(param1.compare("tell_gender_pose") == 0){
			ss.str("");
			if (currentName == "no_gender_pose")
				ss << "I did not found any person";
			else
				ss << "I found a "<< currentName << " person";
			JustinaHRI::waitAfterSay(ss.str(), 2000);
			responseMsg.successful = 1;
		}
		else if(param1.compare("tell_how_many_people") == 0){
			ss.str("");
			if (currentName == "men" || currentName == "boys" || currentName == "male")
				ss << "I found " << men << " " << currentName;
			else if(currentName == "women" || currentName == "girls" || currentName == "female")
				ss << "I found " << women << " " << currentName;
			else if (currentName == "sitting")
				ss << "I found " << sitting << " " << currentName << " people";
			else if (currentName == "standing")
				ss << "I found " << standing << " " << currentName << " people";
			else if (currentName == "lying")
				ss << "I found " << lying << " " << currentName << " people";
			
			JustinaHRI::waitAfterSay(ss.str(), 2000);
			responseMsg.successful = 1;
		}
		else if(param1.compare("ask_name") == 0){
			ss.str("");
			if(numberAttemps == 0){
			JustinaHRI::loadGrammarSpeechRecognized("name_response.xml");
			JustinaHRI::waitAfterSay("Hello my name is Justina", 10000);
			JustinaHRI::waitAfterSay("tell me, my name is, in order to response my question", 10000);}
			JustinaHRI::waitAfterSay("Well, tell me what is your name please", 10000);
			/// codigo para preguntar nombre Se usara un servicio
			bool success = ros::service::waitForService("spg_say", 5000);
			success = success & ros::service::waitForService("/planning_clips/ask_name",5000);
			if (success) {
				knowledge_msgs::planning_cmd srv;
				srv.request.name = "test_ask_name";
				srv.request.params = responseMsg.params;
				if (srvCltAskName.call(srv)) {
					std::cout << "Response of confirmation:" << std::endl;
					std::cout << "Success:" << (long int) srv.response.success << std::endl;
					std::cout << "Args:" << srv.response.args << std::endl;
					currentName = srv.response.args;
					if (srv.response.success){
						ss << "Hello " << srv.response.args;
						JustinaHRI::waitAfterSay(ss.str(), 2000);
					}
					else{
						//success = false;
						//std::cout << "TEST FOR SUCCES VALUE: " << success << std::endl;
						JustinaHRI::waitAfterSay("Could you repeat your name please", 10000);
					}

					//responseMsg.params = srv.response.args;
					responseMsg.successful = srv.response.success;
				} else {
					std::cout << testPrompt << "Failed to call service of confirmation" << std::endl;
					responseMsg.successful = 0;
					JustinaHRI::waitAfterSay("Repeate the command please", 10000);
					responseMsg.successful = 0;
				}
			} else {
				std::cout << testPrompt << "Needed services are not available :'(" << std::endl;
				responseMsg.successful = 0;
			}
		}
		else if(param1.compare("tell_name") == 0){
			ss.str("");
			if (currentName == "ask_name_no"){
				ss << "I find the person, but i did not understand the persons name";
				JustinaHRI::waitAfterSay(ss.str(), 10000);
				responseMsg.successful = 1;
				
			}
			else {
				ss << "Hello, the name of the person I found is " << currentName;
				JustinaHRI::waitAfterSay(ss.str(), 10000);
				responseMsg.successful = 1;
			}
		}
	} else
		success = false;

	/*if (success)
		responseMsg.successful = 1;
	else
		responseMsg.successful = 0;*/

	std::cout << "TEST FOR SUCCES VALUE: " << success << std::endl;
	JustinaHRI::loadGrammarSpeechRecognized(cat_grammar);

	weekdays.clear();
	months.clear();
	days.clear();
	validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);
}

void callbackCmdFindObject(
		const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command find a object ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));
	std::stringstream ss;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	bool nfp = validateContinuePlan(d.toSec(), fplan);
	int timeout = (fplan == true) ? (maxTime - (int)d.toSec() )*1000 : maxTime * 1000;
	std::cout << "TIMEOUT: " << timeout <<std::endl;
	
	bool success = ros::service::waitForService("spg_say", 5000);
	if (success && nfp) {
		std::cout << testPrompt << "find: " << tokens[0] << std::endl;

		ss.str("");
		if (tokens[0] == "person") {
			success = JustinaTasks::findPerson();
			ss << responseMsg.params << " " << 1 << " " << 1 << " " << 1;
		} else if (tokens[0] == "man") {
			JustinaHRI::loadGrammarSpeechRecognized("follow_confirmation.xml");
			if(tokens[1] == "no_location")
				success = JustinaTasks::followAPersonAndRecogStop("stop follow me");
			else
				success = JustinaTasks::findAndFollowPersonToLoc(tokens[1]);
			ss << responseMsg.params;
		} else if (tokens[0] == "man_guide") {
			JustinaNavigation::moveDistAngle(0, 3.1416 ,2000);
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
			success = JustinaTasks::guideAPerson(tokens[1], timeout);
			ss << responseMsg.params;
		} else if (tokens[0] == "specific") {
			success = JustinaTasks::findPerson(tokens[1]);//success = JustinaTasks::findPerson(tokens[1])
			ss << "find_spc_person " << tokens[0] << " " << tokens[1];//ss << responseMsg.params;
		} else if (tokens[0] == "only_find"){
			bool withLeftOrRightArm;
			ss.str("");
			ss << "I am looking for " << tokens[1] << " on the " << tokens[2];
			geometry_msgs::Pose pose;
			JustinaTasks::alignWithTable(0.42);
			boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
			JustinaHRI::waitAfterSay(ss.str(), 2500);
			success = JustinaTasks::findObject(tokens[1], pose, withLeftOrRightArm);
			ss.str("");
			ss << tokens[1] << " " << pose.position.x << " " << pose.position.y << " " << pose.position.z ;
		} else {
			geometry_msgs::Pose pose;
			bool withLeftOrRightArm;
			bool finishMotion = false;
			float pos = 0.0, advance = 0.3, maxAdvance = 0.3;
			do{
				success = JustinaTasks::findObject(tokens[0], pose, withLeftOrRightArm);
				pos += advance;
				if ( pos == maxAdvance && !success){
					JustinaNavigation::moveLateral(advance, 2000);
					boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
					advance = -2 * advance;
				}
				if (pos == -1 * maxAdvance && !success){
					JustinaNavigation::moveLateral(advance, 2000);
					boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
				}
				if (pos == -3 *maxAdvance && !success){
					JustinaNavigation::moveLateral(0.3, 2000);
					boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
					finishMotion = true;}
			}while(!finishMotion && !success);

			if(withLeftOrRightArm)
				ss << responseMsg.params << " " << pose.position.x << " " << pose.position.y << " " << pose.position.z << " left";
			else
				ss << responseMsg.params << " " << pose.position.x << " " << pose.position.y << " " << pose.position.z << " right";
		}
		responseMsg.params = ss.str();
	}
	JustinaHRI::loadGrammarSpeechRecognized(cat_grammar);
	if (success)
		responseMsg.successful = 1;
	else
		responseMsg.successful = 0;
	if(tokens[0] == "only_find"){
		if(nfp) command_response_pub.publish(responseMsg);}
	else
		{if(nfp) validateAttempsResponse(responseMsg);}
	
}

void callbackFindCategory(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg)
{
	std::cout << testPrompt << "-------- Command Find Category--------" << std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));
	std::stringstream ss;

	std::map<std::string, std::string > catList;
	
	catList["candy"] = "snack";
	catList["chewing_gum"] = "snack";
	catList["chup_star"] = "snack";
	catList["curry"] = "snack";
	catList["fries"] = "snack";
	catList["jelly"] = "snack";

	catList["chopstick"] = "cutlery";
	catList["fork"] = "cutlery";
	catList["spoon"] = "cutlery";

	catList["bread"] = "food";
	catList["corn"] = "food";
	catList["onion"] = "food";
	catList["radish"] = "food";

	catList["aquarius"] = "drink";
	catList["cold_brew"] = "drink";
	catList["coke"] = "drink";
	catList["green_tea"] = "drink:";

	catList["asience"] = "cleaning_stuff";
	catList["hair_spray"] = "cleaning_stuff";
	catList["moisturizer"] = "cleaning_stuff";
	catList["shampoo"] = "cleaning_stuff";

	catList["bowl"] = "container";
	catList["soup_container"] = "container";
	catList["plate"] = "container";
	
	catList["apple"] = "fruit";
	catList["orange"] = "fruit";

	bool finishMotion = false;
	float pos = 0.0, advance = 0.3, maxAdvance = 0.3;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	
	ss.str("");
	ss << "I am looking for objects on the " << tokens[1];
	JustinaHRI::waitAfterSay(ss.str(), 2500);
	JustinaManip::hdGoTo(0, -0.9, 5000);
	boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
	JustinaTasks::alignWithTable(0.42);
	boost::this_thread::sleep(boost::posix_time::milliseconds(1000));


	std::map<std::string, int> countCat;
	countCat["snack"] = 0;
	countCat["fruit"] = 0;
	countCat["food"] = 0;
	countCat["drink"] = 0;
	countCat["cleaning_stuff"] = 0;
	countCat["container"] = 0;

	int arraySize = 0;
	int numObj  = 0;

	do{
		boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		std::vector<vision_msgs::VisionObject> recognizedObjects;
		std::cout << "Find a object " << std::endl;
		bool found = 0;
		for (int j = 0; j < 10; j++) {
			std::cout << "Test object" << std::endl;
			found = JustinaVision::detectObjects(recognizedObjects);
			int indexFound = 0;
			if (found) {
				found = false;
				for (int i = 0; i < recognizedObjects.size(); i++) {
					vision_msgs::VisionObject vObject = recognizedObjects[i];
					std::cout << "object:  " << vObject.id << std::endl;
					std::map<std::string, std::string>::iterator it = catList.find(vObject.id);
					if (it != catList.end() && it->second == tokens[0]){
						std::map<std::string, int>::iterator ap = countCat.find(it->second);
						ap->second = ap->second + 1;
						arraySize++;
					}
				}
				if(arraySize > numObj)
					numObj = arraySize;
				arraySize = 0;
			}
		}
		pos += advance;
		if ( pos == maxAdvance){
			JustinaNavigation::moveLateral(advance, 2000);
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
			advance = -2 * advance;
		}
		if (pos == -1 * maxAdvance){
			JustinaNavigation::moveLateral(advance, 2000);
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
		}
		if (pos == -3 *maxAdvance){
			JustinaNavigation::moveLateral(0.3, 2000);
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
			finishMotion = true;}
	}while(!finishMotion && numObj<1);

	ss.str("");
	currentName = tokens[0];
	std::map<std::string, int>::iterator catRes = countCat.find(tokens[0]);
	if(numObj > 0){
		ss << "I found the " << tokens[0];
		JustinaHRI::waitAfterSay(ss.str(), 2500);
		ss.str("");
		ss << responseMsg.params << " " << numObj;
		cantidad = numObj;
		currentName = tokens[0];
		responseMsg.params = ss.str();
		responseMsg.successful = 1;
	}
	else {
		ss << "I can not find the " << tokens[0];
		JustinaHRI::waitAfterSay(ss.str(), 2500);
		ss.str("");
		cantidad = 0;
		ss << responseMsg.params << " " << 0;
		responseMsg.params = ss.str();
		responseMsg.successful = 0;
	}

	//validateAttempsResponse(responseMsg);
	command_response_pub.publish(responseMsg);
	catList.clear();
	countCat.clear();
}

void callbackManyObjects(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg)
{
	std::cout << testPrompt << "-------- Command How Many Objects--------" << std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));
	std::stringstream ss;

	std::map<std::string, int > countObj;
	
	countObj["candy"] = 0;
	countObj["chewing_gum"] = 0;
	countObj["cup_star"] = 0;
	countObj["curry"] = 0;
	countObj["fries"] = 0;
	countObj["jelly"] = 0;

	countObj["aquarius"] = 0;
	countObj["coke"] = 0;
	countObj["cold_brew"] = 0;
	countObj["green_tea"] = 0;

	countObj["bread"] = 0;
	countObj["corn"] = 0;
	countObj["onion"] = 0;
	countObj["radish"] = 0;

	countObj["apple"] = 0;
	countObj["orange"] = 0;

	countObj["asience"] = 0;
	countObj["hair_spray"] = 0;
	countObj["moisturizer"] = 0;
	countObj["shampoo"] = 0;

	countObj["bowl"] = 0;
	countObj["soup_container"] = 0;
	countObj["plate"] = 0;

	countObj["chopstick"] = 0;
	countObj["fork"] = 0;
	countObj["spoon"] = 0;

	countObj["milk"] = 0;
	countObj["juice"] = 0;

	int arraySize = 0;
	int numObj = 0;

	bool finishMotion = false;
	float pos = 0.0, advance = 0.3, maxAdvance = 0.3;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	
	ss.str("");
	ss << "I am looking for the " << tokens[0];
	JustinaHRI::waitAfterSay(ss.str(), 2500);
	
	JustinaManip::hdGoTo(0, -0.9, 5000);
	boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
	JustinaTasks::alignWithTable(0.42);

	do{
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
		std::vector<vision_msgs::VisionObject> recognizedObjects;
		std::cout << "Find a object " << std::endl;
		bool found = 0;
		for (int j = 0; j < 10; j++) {
			std::cout << "Test object" << std::endl;
			found = JustinaVision::detectObjects(recognizedObjects);
			int indexFound = 0;
			if (found) {
				found = false;
				for (int i = 0; i < recognizedObjects.size(); i++) {
					vision_msgs::VisionObject vObject = recognizedObjects[i];
					std::cout << "object:  " << vObject.id << std::endl;
					std::map<std::string, int>::iterator it = countObj.find(vObject.id);
					if (it != countObj.end() && vObject.id == tokens[0]){
						it->second = it->second + 1;
						arraySize++;
					}
					std::cout << "ITERADOR: " << it->second << std::endl;
				}
				if(arraySize > numObj)
					numObj = arraySize;
				arraySize = 0;
			}
		}
		pos += advance;
		if ( pos == maxAdvance){
			JustinaNavigation::moveLateral(advance, 2000);
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
			advance = -2 * advance;
		}
		if (pos == -1 * maxAdvance){
			JustinaNavigation::moveLateral(advance, 2000);
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
		}
		if (pos == -3 *maxAdvance){
			JustinaNavigation::moveLateral(0.3, 2000);
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
			finishMotion = true;}
	}while (!finishMotion);

	ss.str("");
	std::map<std::string, int>::iterator objRes = countObj.find(tokens[0]);
	currentName = tokens[0];
	if(numObj > 0){
		ss << "I found the " << tokens[0];
		JustinaHRI::waitAfterSay(ss.str(), 2500);
		ss.str("");
		cantidad = numObj;
		ss << responseMsg.params << " " << cantidad;
		responseMsg.params = ss.str();
		responseMsg.successful = 1;
	}
	else {
		ss << "I can not find the " << tokens[0];
		JustinaHRI::waitAfterSay(ss.str(),2500);
		ss.str("");
		cantidad = 0;
		ss << responseMsg.params << " " << 0;
		responseMsg.params = ss.str();
		responseMsg.successful = 0;
	}

	validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);
	countObj.clear();
	
}

void callbackOpropObject(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg){
	std::cout << testPrompt << "--------- Command The Oprop Object on the placement ---------" << std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));
	std::stringstream ss;

	std::map<std::string, std::pair<std::string, int> > countObj;
	
	countObj["candy"] = std::make_pair(std::string("snack"),0);
	countObj["chewing_gum"] = std::make_pair(std::string("snack"),0);
	countObj["cup_star"] = std::make_pair(std::string("snack"),0);
	countObj["curry"] = std::make_pair(std::string("snack"),0);
	countObj["fries"] = std::make_pair(std::string("snack"),0);
	countObj["jelly"] = std::make_pair(std::string("snack"),0);

	countObj["bread"] = std::make_pair(std::string("food"),0);
	countObj["corn"] = std::make_pair(std::string("food"),0);
	countObj["onion"] = std::make_pair(std::string("food"),0);
	countObj["radish"] = std::make_pair(std::string("food"),0);

	countObj["aquarius"] = std::make_pair(std::string("drink"),0);
	countObj["cold_brew"] = std::make_pair(std::string("drink"),0);
	countObj["coke"] = std::make_pair(std::string("drink"),0);
	countObj["green_tea"] = std::make_pair(std::string("drink"),0);

	countObj["shampoo"] = std::make_pair(std::string("cleaning_stuff"),0);
	countObj["asience"] = std::make_pair(std::string("cleanning_stuff"),0);
	countObj["hair_spray"] = std::make_pair(std::string("cleanning_stuff"),0);
	countObj["moisturizer"] = std::make_pair(std::string("cleanning_stuff"),0);

	countObj["bowl"] = std::make_pair(std::string("container"),0);
	countObj["soup_container"] = std::make_pair(std::string("container"),0);
	countObj["plate"] = std::make_pair(std::string("container"),0);

	countObj["apple"] = std::make_pair(std::string("fruit"),0);
	countObj["orange"] = std::make_pair(std::string("fruit"),0);

	countObj["chopstick"] = std::make_pair(std::string("cutlery"),0);
	countObj["fork"] = std::make_pair(std::string("cutlery"),0);
	countObj["spoon"] = std::make_pair(std::string("cutlery"),0);

	countObj["juice"] = std::make_pair(std::string("fuit"),0);
	countObj["milk"] = std::make_pair(std::string("fruit"),0);

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	
	std::string prop;
	std::string opropObj;
	std::vector<std::string> objects;
	currentName = tokens[0];
	categoryName = tokens[1];

	bool finishMotion = false;
	float pos = 0.0, advance = 0.3, maxAdvance = 0.3;

	if(tokens[0] == "biggest")
		prop = "bigger";
	else if (tokens[0] == "smallest")
		prop = "smaller";
	else if (tokens[0] == "heaviest")
		prop = "heavier";
	else if (tokens[0] == "lightest")
		prop = "lighter";
	else if (tokens[0] == "largest")
		prop = "larger";
	else if (tokens[0] == "thinnest")
		prop = "thinner";

	JustinaHRI::waitAfterSay("I am looking for objects", 2500);
	JustinaManip::hdGoTo(0, -0.9, 5000);
	boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
	JustinaTasks::alignWithTable(0.42);
		
	do{	
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
		std::vector<vision_msgs::VisionObject> recognizedObjects;
		std::cout << "Find a object " << std::endl;
		bool found = 0;
		for (int j = 0; j < 10; j++) {
			std::cout << "Test object" << std::endl;
			found = JustinaVision::detectObjects(recognizedObjects);
			int indexFound = 0;
			if (found) {
				found = false;
				for (int i = 0; i < recognizedObjects.size(); i++) {
					vision_msgs::VisionObject vObject = recognizedObjects[i];
					std::cout << "object:  " << vObject.id << std::endl;
					std::map<std::string, std::pair<std::string, int> >::iterator it = countObj.find(vObject.id);
					if (it != countObj.end())
						it->second.second = it->second.second + 1;
					if (it->second.second == 1 && tokens[1] == "nil"){
						objects.push_back(it->first);
						std::cout << "OBJETO: " << it->first << std::endl;
					}
					if (it->second.second == 1 && tokens[1] != "nil" && tokens[1] == it->second.first){
						objects.push_back(it->first);
						std::cout << "OBJETO: " << it->first << std::endl;
					}
						
				}
			}
		}
		pos += advance;
		if ( pos == maxAdvance){
			JustinaNavigation::moveLateral(advance, 2000);
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
			advance = -2 * advance;
		}
		if (pos == -1 * maxAdvance){
			JustinaNavigation::moveLateral(advance, 2000);}
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
		if (pos == -3 *maxAdvance){
			JustinaNavigation::moveLateral(0.3, 2000);
			boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
			finishMotion = true;}
	}while(!finishMotion);
	
	
	if(objects.size() == 0){
		objectName = "none";
		responseMsg.successful = 0;
	}

	else if (objects.size() == 1){
		std::cout << "There are only one object" << std::endl;
		objectName = objects.at(0);
		responseMsg.successful = 1;
	}

	bool success = ros::service::waitForService("/planning_clips/str_query_KDB",5000);
	if (success && objects.size() > 1) {
		knowledge_msgs::StrQueryKDB srv;
		objectName = objects.at(0);
		for(int i=1; i<objects.size(); i++){
			ss.str("");
			ss << "(assert (cmd_compare " << prop << " " << objectName << " " << objects.at(i) << " 1))";
			srv.request.query = ss.str();
			if (srvCltQueryKDB.call(srv)) {
				std::cout << "Response of KBD Query:" << std::endl;
				std::cout << "TEST QUERY Args:" << srv.response.result << std::endl;
				str = srv.response.result;
				split(tokens, str, is_any_of(" "));
				objectName = tokens[1];
				responseMsg.successful = 1;
			} else {
				std::cout << testPrompt << "Failed to call service of KBD query"<< std::endl;
				responseMsg.successful = 0;
			}
		}
	}

	command_response_pub.publish(responseMsg);
	countObj.clear();
}

void callbackGesturePerson(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg){
	std::cout << testPrompt << "--------- Command Find the Gender,Gesture or Pose Person ---------" << std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));
	std::stringstream ss;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	bool nfp = validateContinuePlan(d.toSec(), fplan);
	
	if(tokens[0] == "waving"){
		std::cout << "Searching waving person" << std::endl;
		if(nfp) JustinaTasks::findGesturePerson(tokens[0]);
	}
	else if (tokens[0] == "rising_right_arm"){
		std::cout << "Searching rising_right_arm person" << std::endl;
		if(nfp) JustinaTasks::findGesturePerson("right_hand_rised");
	}
	else if (tokens[0] == "rising_left_arm"){
		std::cout << "Searching rising_left_arm person" << std::endl;
		if(nfp) JustinaTasks::findGesturePerson("left_hand_rised");
	}
	else if (tokens[0] == "pointing_right"){
		std::cout << "Searching pointing_right person" << std::endl;
		if(nfp) JustinaTasks::findGesturePerson(tokens[0]);
	}
	else if (tokens[0] == "pointing_left"){
		std::cout << "Searching pointing_left person" << std::endl;
		if(nfp) JustinaTasks::findGesturePerson(tokens[0]);
	}
	else if (tokens[0] == "sitting"){
		std::cout << "Searching sitting person" << std::endl;
		if(nfp) JustinaTasks::findPerson("", -1, JustinaTasks::SITTING);
	}
	else if (tokens[0] == "standing"){
		std::cout << "Searching standing person" << std::endl;
		if(nfp) JustinaTasks::findPerson("", -1, JustinaTasks::STANDING);
	}
	else if (tokens[0] == "lying"){
		std::cout << "Searching lying person" << std::endl;
		if(nfp) JustinaTasks::findPerson("", -1, JustinaTasks::LYING);
	}
	else if (tokens[0] == "man"|| tokens[0] == "boy" || tokens[0] == "male_person"){
		std::cout << "Searching man person" << std::endl;
		if(nfp) JustinaTasks::findPerson("", 1);
	}
	else if (tokens[0] == "woman" || tokens[0] == "girl" || tokens[0] == "female_person"){
		std::cout << "Searching woman person" << std::endl;
		if(nfp) JustinaTasks::findPerson("", 0);
	}

	
	//success = JustinaTasks::findPerson();

	if(nfp) command_response_pub.publish(responseMsg);
}

void callbackGPPerson(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg){
	std::cout << testPrompt << "--------- Command Find which gender or pose have the person ---------" << std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));
	std::stringstream ss;
	std::string gender;
	bool success;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;

	ss.str("");	
	if(tokens[0] == "gender"){
		std::cout << "Searching person gender" << std::endl;
		success = JustinaTasks::tellGenderPerson(gender);
		if (success){
			ss << "you are a " << gender; 
			std::cout << "Genero " << gender << std::endl; 
			currentName = gender;
			JustinaHRI::waitAfterSay(ss.str(), 10000);
			}
		else
			currentName = "no_gender_pose";
	}
	else if (tokens[0] == "pose"){std::cout << "Searching person pose" << std::endl;
			JustinaTasks::findPerson();
			currentName = "standing";
			ss << "you are " << currentName;
			JustinaHRI::waitAfterSay(ss.str(), 10000);
	}

	command_response_pub.publish(responseMsg);
}

void callbackGPCrowd(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg){
	std::cout << testPrompt << "--------- Command Find which gender or pose have the crowd ---------" << std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));
	std::stringstream ss;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	
	women = 0;
	men = 0;
	sitting = 0;
	standing = 0;
	lying = 0;
	
	JustinaTasks::findCrowd(men, women, sitting, standing, lying);
	
	currentName = tokens[0];

	ss.str("");	
	if(tokens[0] == "men"){std::cout << "Searching person men" << std::endl;
		ss << "I found " << men << " men";
		JustinaHRI::waitAfterSay(ss.str(), 10000);
	}
	else if (tokens[0] == "women"){std::cout << "Searching women in the crowd" << std::endl;
		ss << "I found " << women << " women";
		JustinaHRI::waitAfterSay(ss.str(), 10000);}
	else if (tokens[0] == "boys"){std::cout << "Searching boys in the crowd" << std::endl;
		ss << "I found " << men << " boys";
		JustinaHRI::waitAfterSay(ss.str(), 10000);}
	else if (tokens[0] == "girls"){std::cout << "Searching girls in the crowd" << std::endl;
		ss << "I found " << women << " girls";
		JustinaHRI::waitAfterSay(ss.str(), 10000);}
	else if (tokens[0] == "male"){std::cout << "Searching male in the crowd" << std::endl;
		ss << "I found " << men << " male people";
		JustinaHRI::waitAfterSay(ss.str(), 10000);}
	else if (tokens[0] == "famale"){std::cout << "Searching female in the crowd" << std::endl;
		ss << "I found " << men << " female";
		JustinaHRI::waitAfterSay(ss.str(), 10000);}
	else if (tokens[0] == "sitting"){std::cout << "Searching sitting in the crowd" << std::endl;
		ss << "I found " << sitting << " sitting people";
		JustinaHRI::waitAfterSay(ss.str(), 10000);}
	else if (tokens[0] == "standing"){std::cout << "Searching standing in the crowd" << std::endl;
		ss << "I found " << standing << " standing people";
		JustinaHRI::waitAfterSay(ss.str(), 10000);}
	else if (tokens[0] == "lying"){std::cout << "Searching lying in the crowd" << std::endl;
		ss << "I found " << lying << " lying people";
		JustinaHRI::waitAfterSay(ss.str(), 10000);}


	command_response_pub.publish(responseMsg);
}

void callbackCmdAskIncomplete(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg){
	std::cout << testPrompt << "--------- Command ask for incomplete information ---------" << std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));
	std::stringstream ss;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	
	
	ss.str("");
	if(tokens[0] == "follow_place_origin"){
		JustinaHRI::loadGrammarSpeechRecognized("incomplete_place.xml");
		ss << "Well, tell me where can i find " << tokens[2]; 
		JustinaHRI::waitAfterSay(" in order to response my question, Say for instance, at the center table", 10000);
		JustinaHRI::waitAfterSay(ss.str(), 10000);}	
	if(tokens[0] == "gesture_place_origin"){
		JustinaHRI::loadGrammarSpeechRecognized("incomplete_place.xml");
		ss << "Well, tell me where can i find a " << tokens[2] << " person"; 
		JustinaHRI::waitAfterSay(" in order to response my question, Say for instance, at the center table", 10000);
		JustinaHRI::waitAfterSay(ss.str(), 10000);}	
	if(tokens[0] == "object"){
		JustinaHRI::loadGrammarSpeechRecognized("incomplete_object.xml");
		ss << "Well, tell me what " << tokens[2] << " do you want";
		JustinaHRI::waitAfterSay(" in order to response my question, Say for instance, I want pringles", 10000);
		JustinaHRI::waitAfterSay(ss.str(), 10000);}
	if(tokens[0] == "place_destiny"){
		JustinaHRI::loadGrammarSpeechRecognized("incomplete_place.xml");
		JustinaHRI::waitAfterSay(" in order to response my question, Say for instance, at the living table", 10000);
		JustinaHRI::waitAfterSay("Well, tell me where is the destiny location", 10000);}
	ss.str("");
		
	/// codigo para preguntar nombre Se usara un servicio
	bool success = ros::service::waitForService("spg_say", 5000);
	success = success & ros::service::waitForService("/planning_clips/ask_incomplete",5000);
	if (success) {
		knowledge_msgs::planning_cmd srv;
		srv.request.name = "test_ask_place";
		srv.request.params = responseMsg.params;
		if (srvCltAskIncomplete.call(srv)) {
			std::cout << "Response of confirmation:" << std::endl;
			std::cout << "Success:" << (long int) srv.response.success << std::endl;
			std::cout << "Args:" << srv.response.args << std::endl;
			currentName = srv.response.args;
			if (srv.response.success){
				if(tokens[0] == "follow_place_origin")
					ss << "Well i will find the person in the " << srv.response.args;
				else if(tokens[0] == "object")
					ss << "well i will find the " << srv.response.args;
				else if(tokens[0] == "place_destiny")
					ss << "well i will guide the person to the " << srv.response.args;
				JustinaHRI::waitAfterSay(ss.str(), 2000);
			}
			else{
				if(tokens[0] == "follow_place_origin")
					JustinaHRI::waitAfterSay("Could you repeat in wich place is the person please", 10000);
				if(tokens[0] == "object")
					JustinaHRI::waitAfterSay("Could you repeat what object do you want please", 10000);
				if(tokens[0] == "place_destiny")
					JustinaHRI::waitAfterSay("Could you repeat the destiny place please", 10000);
			}
			ss.str("");
			ss << msg->params << " " << srv.response.args;
			responseMsg.params = ss.str();
			responseMsg.successful = srv.response.success;
		} else {
			std::cout << testPrompt << "Failed to call service of confirmation" << std::endl;
			responseMsg.successful = 0;
			JustinaHRI::waitAfterSay("Repeate the command please", 10000);
			responseMsg.successful = 0;
		}
	} else {
		std::cout << testPrompt << "Needed services are not available :'(" << std::endl;
		responseMsg.successful = 0;
	}



	command_response_pub.publish(responseMsg);
	JustinaHRI::loadGrammarSpeechRecognized(cat_grammar);
}

void callbackAskFor(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command Ask for ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	
	/*std::stringstream ss;
	 ss << responseMsg.params << " " << "table";
	 responseMsg.params = ss.str();*/
	responseMsg.successful = 1;
	validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);
}

void callbackStatusObject(
		const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command Status object ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::stringstream ss;
	ss << responseMsg.params << " " << "open";

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	
	bool success = JustinaTasks::alignWithTable(0.42);

	if (success)
		responseMsg.successful = 1;
	else
		responseMsg.successful = 0;

	responseMsg.params = ss.str();
	responseMsg.successful = 1;
	validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);
}

void callbackMoveActuator(
		const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command Move actuator ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));
	bool armFlag = true;
	std::stringstream ss;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	
	bool success = ros::service::waitForService("spg_say", 5000);
	//success = success & tasks.moveActuator(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()), tokens[0]);
	if(tokens[4] == "false")
			armFlag = false;

	//ss << "I try to grasp the " << tokens[0];
	//JustinaHRI::waitAfterSay(ss.str(), 10000);
	
	success = success & JustinaTasks::moveActuatorToGrasp(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()), armFlag, tokens[0], true);
	if (success)
		responseMsg.successful = 1;
	else{
		ss.str("");
		ss << "I did not grasp the " << tokens[0];
		JustinaHRI::waitAfterSay(ss.str(), 100000);
		responseMsg.successful = 0;
	}

	validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);
}

void callbackDrop(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command Drop ---------" << std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	std::vector<std::string> tokens;
	std::string str = responseMsg.params;
	split(tokens, str, is_any_of(" "));
	std::stringstream ss;
	bool armFlag = true;
	bool succes;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	bool nfp = validateContinuePlan(d.toSec(), fplan);
	
	if(tokens[2] == "false")
			armFlag = false;

	if(tokens[0] == "person" && nfp)
		succes = JustinaTasks::dropObject(tokens[1], armFlag, 30000);
	else if(tokens[0] == "object" && nfp ){
		ss.str("");
		ss << "I am going to deliver the " << tokens[1];
		JustinaHRI::waitAfterSay(ss.str(), 2000);
		succes = JustinaTasks::placeObject(armFlag);
		(armFlag) ? JustinaManip::laGoTo("home", 6000) : JustinaManip::raGoTo("home", 6000);
	}
	
	if (succes)
		responseMsg.successful = 1;
	else
		responseMsg.successful = 0;

	if(nfp) validateAttempsResponse(responseMsg);
}

void callbackUnknown(const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command unknown ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	responseMsg.successful = 1;
	validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);
}


void callbackAskPerson(
		const knowledge_msgs::PlanningCmdClips::ConstPtr& msg) {
	std::cout << testPrompt << "--------- Command ask for person ---------"
			<< std::endl;
	std::cout << "name:" << msg->name << std::endl;
	std::cout << "params:" << msg->params << std::endl;

	knowledge_msgs::PlanningCmdClips responseMsg;
	responseMsg.name = msg->name;
	responseMsg.params = msg->params;
	responseMsg.id = msg->id;

	ros::Time finishPlan = ros::Time::now();
	ros::Duration d = finishPlan - beginPlan;
	std::cout << "TEST PARA MEDIR EL TIEMPO: " << d.toSec() << std::endl;
	
	bool success = ros::service::waitForService("spg_say", 5000);
	success = success
			& ros::service::waitForService("/planning_clips/confirmation",
					5000);
	JustinaManip::startHdGoTo(0, 0.0);
	if (success) {
		std::string to_spech = responseMsg.params;
		boost::replace_all(to_spech, "_", " ");
		std::stringstream ss;
		ss << "Hello, Tell me robot yes, or robot no in order to response my question, Well, Is your name, " << to_spech;
		//JustinaHRI::waitAfterSay(ss.str(), 1500);
		//ss << "Well, " << to_spech << " is your name";
		std::cout << "------------- to_spech: ------------------ " << ss.str() << std::endl;

		JustinaHRI::waitAfterSay(ss.str(), 10000);

		knowledge_msgs::planning_cmd srv;
		srv.request.name = "test_confirmation";
		srv.request.params = responseMsg.params;
		if (srvCltWaitConfirmation.call(srv)) {
			std::cout << "Response of confirmation:" << std::endl;
			std::cout << "Success:" << (long int) srv.response.success
					<< std::endl;
			std::cout << "Args:" << srv.response.args << std::endl;
			if (srv.response.success){
				ss.str("");
				ss << "Hello " << to_spech;
				JustinaHRI::waitAfterSay(ss.str(),1500);
				
			}
			else{
				ss.str("");
				ss << to_spech << ", I try to find you again ";
				JustinaHRI::waitAfterSay(ss.str(), 1500);
				boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
				JustinaNavigation::moveDistAngle(0, 1.57, 10000);
				boost::this_thread::sleep(boost::posix_time::milliseconds(4000));
			}

			responseMsg.params = responseMsg.params;//srv.response.args;
			responseMsg.successful = srv.response.success;
		} else {
			std::cout << testPrompt << "Failed to call service of confirmation"
					<< std::endl;
			responseMsg.successful = 0;
			JustinaHRI::waitAfterSay("Repeate the command please", 2000);
		}
	} else {
		std::cout << testPrompt << "Needed services are not available :'("
				<< std::endl;
		responseMsg.successful = 0;
	}
	validateAttempsResponse(responseMsg);
	//command_response_pub.publish(responseMsg);
}

int main(int argc, char **argv) {

	ros::init(argc, argv, "gpsr_test");
	ros::NodeHandle n;
	ros::Rate rate(10);

	srvCltGetTasks = n.serviceClient<knowledge_msgs::planning_cmd>("/planning_clips/get_task");
	srvCltInterpreter = n.serviceClient<knowledge_msgs::planning_cmd>("/planning_clips/spr_interpreter");
	srvCltWaitConfirmation = n.serviceClient<knowledge_msgs::planning_cmd>("/planning_clips/confirmation");
	srvCltWaitForCommand = n.serviceClient<knowledge_msgs::planning_cmd>("/planning_clips/wait_command");
	srvCltAnswer = n.serviceClient<knowledge_msgs::planning_cmd>("/planning_clips/answer");
	srvCltAskName = n.serviceClient<knowledge_msgs::planning_cmd>("/planning_clips/ask_name");
	srvCltAskIncomplete = n.serviceClient<knowledge_msgs::planning_cmd>("/planning_clips/ask_incomplete");
	srvCltQueryKDB = n.serviceClient<knowledge_msgs::StrQueryKDB>("/planning_clips/str_query_KDB");

	ros::Subscriber subCmdSpeech = n.subscribe("/planning_clips/cmd_speech", 1, callbackCmdSpeech);
	ros::Subscriber subCmdInterpret = n.subscribe("/planning_clips/cmd_int", 1, callbackCmdInterpret);
	ros::Subscriber subCmdConfirmation = n.subscribe("/planning_clips/cmd_conf", 1, callbackCmdConfirmation);
	ros::Subscriber subCmdGetTasks = n.subscribe("/planning_clips/cmd_task", 1, callbackCmdGetTasks);

	ros::Subscriber subCmdNavigation = n.subscribe("/planning_clips/cmd_goto", 1, callbackCmdNavigation);
	ros::Subscriber subCmdAnswer = n.subscribe("/planning_clips/cmd_answer", 1, callbackCmdAnswer);
	ros::Subscriber subCmdFindObject = n.subscribe("/planning_clips/cmd_find_object", 1, callbackCmdFindObject);
	ros::Subscriber subCmdAskFor = n.subscribe("/planning_clips/cmd_ask_for", 1, callbackAskFor);
	ros::Subscriber subCmdStatusObject = n.subscribe("/planning_clips/cmd_status_object", 1, callbackStatusObject);
	ros::Subscriber subCmdMoveActuator = n.subscribe("/planning_clips/cmd_move_actuator", 1, callbackMoveActuator);
	ros::Subscriber subCmdDrop = n.subscribe("/planning_clips/cmd_drop", 1, callbackDrop);
	ros::Subscriber subCmdUnknown = n.subscribe("/planning_clips/cmd_unknown", 1, callbackUnknown);
	ros::Subscriber subAskPerson = n.subscribe("/planning_clips/cmd_ask_person", 1, callbackAskPerson);
	ros::Subscriber subFindCategory = n.subscribe("/planning_clips/cmd_find_category", 1, callbackFindCategory);
	ros::Subscriber subManyObjects = n.subscribe("/planning_clips/cmd_many_obj", 1, callbackManyObjects);
	ros::Subscriber subPropObj = n.subscribe("/planning_clips/cmd_prop_obj", 1, callbackOpropObject);
	ros::Subscriber subGesturePerson = n.subscribe("/planning_clips/cmd_gesture_person", 1, callbackGesturePerson);
	ros::Subscriber subGPPerson = n.subscribe("/planning_clips/cmd_gender_pose_person", 1, callbackGPPerson);
	ros::Subscriber subGPCrowd = n.subscribe("/planning_clips/cmd_gender_pose_crowd", 1, callbackGPCrowd);
	ros::Subscriber subSpeechGenerator = n.subscribe("/planning_clips/cmd_speech_generator", 1, callbackCmdSpeechGenerator);
	ros::Subscriber subAskIncomplete = n.subscribe("/planning_clips/cmd_ask_incomplete", 1, callbackCmdAskIncomplete);

	command_response_pub = n.advertise<knowledge_msgs::PlanningCmdClips>("/planning_clips/command_response", 1);

	JustinaHRI::setNodeHandle(&n);
	JustinaHardware::setNodeHandle(&n);
	JustinaKnowledge::setNodeHandle(&n);
	JustinaManip::setNodeHandle(&n);
	JustinaNavigation::setNodeHandle(&n);
	JustinaTasks::setNodeHandle(&n);
	JustinaTools::setNodeHandle(&n);
	JustinaVision::setNodeHandle(&n);
	JustinaRepresentation::setNodeHandle(&n);
	
	JustinaRepresentation::initKDB("", false, 20000);

	if (argc > 3){
		std::cout << "FPLAN FLAG: " << argv[3] << std::endl;
		fplan = atoi(argv[3]);
		maxTime = atof(argv[4]);
		cat_grammar = argv[5];
		std::cout << "FPLAN FLAG: " << fplan << std::endl;
		std::cout << "MAX TIME: " << maxTime << std::endl;
		std::cout << "Grammar: " << cat_grammar << std::endl;}

	while (ros::ok()) {

		switch (state) {
		case SM_INIT:
			if (startSignalSM) {
				JustinaHRI::waitAfterSay("I am ready for the eegpsr test", 4000);
				state = SM_SAY_WAIT_FOR_DOOR;
			}
			break;
		case SM_SAY_WAIT_FOR_DOOR:
			JustinaHRI::waitAfterSay("I am waiting for the door to be open",
					4000);
			state = SM_WAIT_FOR_DOOR;
			break;
		case SM_WAIT_FOR_DOOR:
			if (!JustinaNavigation::obstacleInFront())
				state = SM_NAVIGATE_TO_THE_LOCATION;
			break;
		case SM_NAVIGATE_TO_THE_LOCATION:
			JustinaHRI::waitAfterSay("Now I can see that the door is open",4000);
			std::cout << "GPSRTest.->First try to move" << std::endl;
            JustinaNavigation::moveDist(1.0, 4000);
			if (!JustinaTasks::sayAndSyncNavigateToLoc("arena", 120000)) {
				std::cout << "GPSRTest.->Second try to move" << std::endl;
				if (!JustinaTasks::sayAndSyncNavigateToLoc("arena", 120000)) {
					std::cout << "GPSRTest.->Third try to move" << std::endl;
					if (JustinaTasks::sayAndSyncNavigateToLoc("arena", 120000)) {
						JustinaHRI::waitAfterSay("please tell me robot yes for confirm the command", 10000);
						JustinaHRI::waitAfterSay("please tell me robot no for repeat the command", 10000);
						JustinaHRI::waitAfterSay("I am ready for recieve a category five command", 10000);
						state = SM_SEND_INIT_CLIPS;
					}
				} else {
					JustinaHRI::waitAfterSay("please tell me robot yes for confirm the command", 10000);
					JustinaHRI::waitAfterSay("please tell me robot no for repeat the command", 10000);
					JustinaHRI::waitAfterSay("I am ready for recieve a category five command", 10000);
					state = SM_SEND_INIT_CLIPS;
				}
			} else {
				JustinaHRI::waitAfterSay("please tell me robot yes for confirm the command", 10000);
				JustinaHRI::waitAfterSay("please tell me robot no for repeat the command", 10000);
				JustinaHRI::waitAfterSay("I am ready for recieve a category five command", 10000);
				state = SM_SEND_INIT_CLIPS;
			}
			break;
		case SM_SEND_INIT_CLIPS:
			JustinaVision::startQRReader();
			initMsg.successful = false;
			runSMCLIPS = true;
			command_response_pub.publish(initMsg);
			state = SM_RUN_SM_CLIPS;
			break;
		case SM_RUN_SM_CLIPS:
			break;
		}

		rate.sleep();
		ros::spinOnce();
	}

	JustinaVision::stopQRReader();

	return 0;

}
