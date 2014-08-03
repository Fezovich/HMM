#ifndef File_read_H
#define	File_read_H

#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <conio.h>
#include <stdlib.h>
#include <vector>
#include <map>
using namespace std;

class Markov
{
	vector<string> obsData;
	vector<string> hiddenData;
	
	string userEventString; // global for output
	string states;
	string stateTemp;
	string absorbChar;
	
	map<string, float> initialPrbMap;										// The inital probability map of all possible states.

	map<string, char>obsNameMap;											// Maps the full event name with the assigned character.
	map<string, char>::iterator itobsNameMap;

	map<string, vector<int>> hiddenDataPos;									// Stores the location that each event appears in the input data vector.
	map<string, vector<int>>::iterator itHiddenDataPos;

	map<string, int> hiddenStateIndex;										// Stores the the int location of the events corrisponding to the location within the vectors of the transition map.
	map<string, int>::iterator ithiddenStateIndex;

	map<string, int> obsEventIndex;											// Stores the the int location of the events corrisponding to the location within the vectors of the observation map.

	map<string, vector<float>> obsPrbMap;									// Stores the probability that an observation will be seen for each state.


	map<string, vector<float>> hiddenTrans;									// Stores the state transition probability values as float.
	map<string, vector<float>>::iterator itCurrent;

	map<string, vector<float>> forwardPrbValues;							// Stores the results of the forward calculation.
	map<string, vector<float>> backPrbValues;								// Stores the results of the back calculation.
	map<string, vector<float>> smoothedValues;								// Stores the results of the smoothed calculation.

	vector<int> failLocation;
	int stateLengthMax;
	int absorbFound;
	bool absorbMode;

public:
	void printHiddenTrans();
	void makeEventAllocationsFile();
	void loadStringFile();
	void generateMaps();
	void generateStateMap();
	void calcBayes();
	void calcFwdBkAlg();
	Markov();

private:
	bool ignFlag(string field);
	void speedStateAllocation(string field);

	void convertAndStoreData();

	void userMapDef();

	void loadStateString(int shiftRight,int vecValue);

	void mapKeyStore(int vecLocation);
	void mapKeyPositions(int stateLocation);
	void populateStateMap();
	void IndexSetup(map<string, int>& IndexMap);
	void initialPrbCalc();
	void makeTransitionMap();
	void makeObservationMap();
	void markovValueCheck();

	void obtainBayesValues(float &initialVal, float &obsValue, string State, string Observation);

	void fwdMath(bool firstIteration, bool backCalcFlag, int fwdItValue, int obsLoc, map<string, vector<float>>& FwdBkPrbMap);
	void calcSmoothResults();

	void failFind();
	bool fillAbsorbValues(vector<float>& vecData);
	
};

Markov::Markov(){

	bool userFinished = false;
	char userEnd;

	loadStringFile();
	makeEventAllocationsFile();
	
	userMapDef();
	generateMaps();

	while( userFinished == false){

		calcFwdBkAlg(); 

		if(absorbMode == true){
			calcBayes();
		}
	printHiddenTrans();
		cout << "\n\nEnter y to restart / q to quit ";
		cin >> userEnd;

		if( userEnd == 'q'){
			userFinished = true;
		}
	}
}

void Markov::printHiddenTrans(){

	ofstream myfile;
	myfile.open ("MarkovMap.csv");																	// Start making the file we will store the events and an allocated character in.

	map<string, vector<float> >::iterator itr;
	map<string, float >::iterator tempit;
	char obsEvent;

	myfile << " Initial State \n";

	for(tempit = initialPrbMap.begin(); tempit != initialPrbMap.end(); ++tempit)
	{
		myfile << tempit->first << ": " << tempit->second ;
		myfile << " ";		
	myfile << endl;
	}


	myfile << "\n Transition Map \n";


	for(itr = hiddenTrans.begin(); itr != hiddenTrans.end(); ++itr)
	{
		myfile << itr->first << ": ";
		myfile << " ";
		for(std::vector<float>::iterator j = itr->second.begin(); j != (itr->second).end(); ++j)
		{
		myfile << " " << *j;
		}
	myfile << endl;
	}

	myfile << " Observation Map\n";

	for(itr = obsPrbMap.begin(); itr != obsPrbMap.end(); ++itr)
	{
		myfile << itr->first << ": ";
		myfile << " ";
		for(std::vector<float>::iterator j = itr->second.begin(); j != (itr->second).end(); ++j)
		{
		myfile << " " << *j;
		}
	myfile << endl;
	}


	myfile << " Fwd Back Calc Result\n";


	itr = smoothedValues.begin();
	int itVec= 0;
	string tmpName;

		myfile << itr->first << ": ";
		myfile << "\n";
		for(std::vector<float>::iterator j = itr->second.begin(); j != (itr->second).end(); ++j, ++itVec)
		{

		obsEvent = userEventString[itVec];																//Extract a single event.

		for(itobsNameMap = obsNameMap.begin(); itobsNameMap != obsNameMap.end(); ++itobsNameMap){

			if(itobsNameMap->second == obsEvent){
				tmpName = itobsNameMap->first;
			}
		
		}
		myfile << obsEvent << "," << *j << ","<< tmpName << "\n";
		}
	myfile << endl;
	
	 





	myfile.close();
}

void Markov::loadStringFile(){													// Opens the previously generated file and increments through the string printing and storing each character to the screen
	
	ifstream myfile2 ("VEHICLE_HISTORY_Test_Data.csv");							// open file and assign to string 
	string states;	
	bool headerSkip = true;
	if (myfile2.is_open()){

		while ( myfile2.good()){

			while (getline(myfile2,states)){

				stringstream ss(states);
				string field;

				if(headerSkip == false){

					for(int Column = 0; Column < 3; ++Column){						// Look at the second and third column for an event and speed.

						(getline(ss, field, '\t'));									// Load the data fields seperated by tabs.

						switch(Column){
						case 1:
							obsData.push_back(field);								// Store the event in the observations vector.
							if(ignFlag(field) == true){
								hiddenData.push_back("s");							// If it is an ignition event vehicle must be stationary.
								Column = 3;											// Dont bother looking for a speed value in the next column as there isnt one.
							}
						break;
						case 2:
							if(field.size() == 1){									// If there are multiple tabs between event and speed keep loading data untill speed is found.
								--Column;
							}else{													// Speed is found.
							    speedStateAllocation(field);						// Determin if the vehicle is stationary or moving.
							}
						break;

						}				
					}	
				}
			headerSkip = false;
			}
		}

		myfile2.close();

	}else{

		cout << "Unable to open file states"; 
	}
}

bool Markov::ignFlag(string field){

	string strIgnOff = "Ignition Off";								// Special case with no speed record.
	string strIgnOn = "Ignition On";								// Special case with no speed record.
	string temp;
	// Consider if field 2 = g to do less iterations?
	for(int pos = 0; pos < field.size(); pos++){					// Remove the 0 characters between the letters.		
		if(field[pos] != 0){
			temp = temp + field[pos];								//Store to a new string
		}
	}

	if(strIgnOff == temp || strIgnOn == temp){						//Check if the loaded string matches the Ignition events.
		return true;
	}
	return false;
}

void Markov::speedStateAllocation(string field){

	string temp;

	for(int pos = 0; pos < field.size(); pos++){				// Fillter out the random 0s.
		if(field[pos] != 0){
			temp = temp + field[pos];							// make a string out of the numbers	
		}
	}

	int Result = 0;
	stringstream convert(temp);									// Load into stringstream for value converison.							
	convert>> Result;											// Convert to an Int.

	if(Result > 0){												// Decide if moving or stationary.
		hiddenData.push_back("m");								// Moving.
		}else{
		hiddenData.push_back("s");								// Stationary.
		}


}

void Markov::makeEventAllocationsFile(){															// Convert long string event names to single characters.										

	char letter = 'A';

	for (int itVec = 0; itVec != obsData.size(); ++itVec){											// Store all events that appeared in the data sample and assign a char that will be changed later.

		obsNameMap[obsData[itVec]] = 'a';		
	}

	ofstream myfile;
	myfile.open ("eventNamevalues.csv");															// Start making the file we will store the events and an allocated character in.

	for(itobsNameMap = obsNameMap.begin(); itobsNameMap != obsNameMap.end(); itobsNameMap++){		//loop through the map with the name of all unique events that appear in the data.

		itobsNameMap->second = letter;																// Assign a letter to the value that will be a unique representation of each event.
		letter++;
		myfile << itobsNameMap->second << " : " << itobsNameMap->first << " \n";					// store the key and the value in a file.
	}

	myfile.close();		
	convertAndStoreData();																			//Convert the events data to the characters we have assigned the events.
	

}																

void Markov::convertAndStoreData(){																		//Convert the data vector from event strings to chars.

	for (int itVec = 0; itVec != obsData.size(); ++itVec){												//Go through the entire event data file that was loaded into the vector obsData.

		for(itobsNameMap = obsNameMap.begin(); itobsNameMap != obsNameMap.end(); itobsNameMap++){		//for each event in the obsData vector look through the entire name map.

			if(itobsNameMap->first == obsData[itVec]){													//If the event in the map key matches the event in the obsData replace the event with the assigned character.
				obsData[itVec] = itobsNameMap->second;
				
			}
		}
	}

	ofstream myfile;
	myfile.open ("TestDataConv1.csv");																	//Make a new data file with the now converted events to characters.

	for (int itVec = 0; itVec != obsData.size(); ++itVec){	
		myfile <<  obsData[itVec];
	}

	myfile.close();
}

void Markov::userMapDef(){

//	cout << "\n\nEnter the size of each state: ";
//	cin >> stateLengthMax;
	stateLengthMax = 1;
	stateTemp.resize(stateLengthMax);														//user defines the length of a state.	

//	cout << "\nIf absorb state exists enter character, else enter \"null\":";
//	cin >> absorbChar;
    absorbChar ='null';
	if(absorbChar.find('null') == string::npos){											//Define if special considerations need to be taken in the map generation.
		absorbMode = true;
	}else{
		absorbMode = false;
	}	 

}

void Markov::generateMaps(){

	for(int generateStage = 1; generateStage < 3; generateStage++){

		for(int itVec = 0; itVec != obsData.size()-(stateLengthMax-1); ++itVec){					//Look through all the events in obsData Vector											
			
			switch(generateStage){
				case 1:
					mapKeyStore(itVec);																//Store the stateTemp's loaded from obsData keys in the Maps.
				break;
				case 2:
					mapKeyPositions(itVec);															//Locate and store the position of key occurances within obsData.	
				break;
			}			
		}
	}
	populateStateMap();																				//Fill the map that will become the Transition Map with blank float vectors of map.size() length.
	IndexSetup(hiddenStateIndex);																	// allocate a number to each map key so we can access the vectors locations faster.
	IndexSetup(obsEventIndex);																		// allocate a number to each map key so we can access the vectors locations faster.
	initialPrbCalc();
	makeTransitionMap();																			// Calculate transition probabilites for hidden states.
	makeObservationMap();																			// Calculate the probabilites of each hidden state key producing each observation event.
	markovValueCheck();																				// Check that each vector adds up to a total value of 1 i.e. 100%
}

void Markov::loadStateString(int shiftRight,int vecValue){

	stateTemp.clear();

	
	vecValue = vecValue + shiftRight;													// Shift right by one to look at the state that comes after our current state.
																						// add out of range detection.
	if( (vecValue + (stateLengthMax-1) + shiftRight) < obsData.size()){

		for(unsigned k = 0; k != stateLengthMax; k++){									// group a number of vector elements stateLengthMax long from the itVec position into a string.		
			//think about making the vector variable.
			stateTemp = stateTemp + hiddenData[vecValue + k];								// add each char to the string to create our state for the map.
		}
	}
}

void Markov::mapKeyStore(int vecLocation){												// Stores the stateLengthMax sized string as a key in the map as long as an absorbing state is not found at a position other than the end.
																							
	//Used to make transition map of hidden states.
	hiddenTrans[hiddenData[vecLocation]] = vector<float>();								// Markov probability map
	forwardPrbValues[hiddenData[vecLocation]] = vector<float>();						// Stores forward calculation results.
	backPrbValues[hiddenData[vecLocation]] = vector<float>();
	smoothedValues[hiddenData[vecLocation]] = vector<float>();

	hiddenDataPos[hiddenData[vecLocation]]= vector<int>();								// location the states appear in the obsData vector.
	hiddenStateIndex[hiddenData[vecLocation]];											// Index for speedy access of the vectors of the probability map.
	initialPrbMap[hiddenData[vecLocation]];

	//Used to make observation probabilty map.
	obsEventIndex[obsData[vecLocation]];												// Make a map of all the Observations and a value we can use to represent the vector in the obsprobmap
	obsPrbMap[hiddenData[vecLocation]];
}

void Markov::mapKeyPositions(int stateLocation){

	if(hiddenDataPos.find(hiddenData[stateLocation]) != hiddenDataPos.end()){					//Make sure the state is in the map. Fixes Issue caused by absorbMode as some states that appear in raw data are ignored.

		itHiddenDataPos = hiddenDataPos.find(hiddenData[stateLocation]); 						//Find the key that matches the loaded state from the raw data.
		itHiddenDataPos->second.push_back(stateLocation);										//Store the value itHiddenDataPos which is pointing to the first character in our loaded string from the obsData Vector into the map value location.																												
		
	}
}

void Markov::populateStateMap(){

	for (itCurrent = hiddenTrans.begin(); itCurrent != hiddenTrans.end(); ++itCurrent){			// Prepare the Markov map with vectors that are the same size as the number of keys in the map.

		vector<float> tempVec(hiddenTrans.size());	
		hiddenTrans[itCurrent->first].swap(tempVec);
	}


	for (itCurrent = obsPrbMap.begin(); itCurrent != obsPrbMap.end(); ++itCurrent){			// Prepare the Markov map with vectors that are the same size as the number of keys in the map.

		vector<float> tempVec(obsEventIndex.size());	
		obsPrbMap[itCurrent->first].swap(tempVec);
	}


}

void Markov::IndexSetup(map<string, int>& IndexMap){											//Assign an int to each key of the map.

	int indexValue = 0;

	for (ithiddenStateIndex = IndexMap.begin(); ithiddenStateIndex != IndexMap.end(); ++ithiddenStateIndex){
	
		IndexMap[ithiddenStateIndex->first] = indexValue;
		indexValue++;
	}

}

void Markov::initialPrbCalc(){																		// Calculate the inital probabilites for the HMM.


	map<string, float>::iterator tempit;
	tempit = initialPrbMap.begin();
	


	for(itHiddenDataPos = hiddenDataPos.begin(); itHiddenDataPos != hiddenDataPos.end(); ++itHiddenDataPos){
	
		float a = itHiddenDataPos->second.size();
		float b = hiddenData.size();
		float probValue = 0;
		
		probValue = (a / b );

		initialPrbMap[tempit->first] = probValue;
		tempit++;
	}


}

void Markov::makeTransitionMap(){																			

	float totalEventCount;

	for(itHiddenDataPos = hiddenDataPos.begin(); itHiddenDataPos != hiddenDataPos.end(); ++itHiddenDataPos){			//Loop through our event positions

		vector<int> tempVecPos;
		vector<float> tempVecData;

		itCurrent = hiddenTrans.find(itHiddenDataPos->first);															//find tha iterator that points to the same stateTemp value in the hiddenTranstore.
		hiddenTrans[itCurrent->first].swap(tempVecData);																//Swap out the vector we need to manipulate.
	
		if(fillAbsorbValues(tempVecData) == false){																		//Absorb skip start. If it is an absorbing state it has 100% chance to transition to itself therefore the rest of the calculation is not required.
		
			hiddenDataPos[itHiddenDataPos->first].swap(tempVecPos);														//Swap out the vector storing all the locations of the key state.
			totalEventCount = tempVecPos.size();																		//Get a total count of how many times this state appears in obsData
			
			for(int itVec = 0; itVec != tempVecPos.size(); ++itVec){													//Look at every possible location that the state appears in obsData

				int statePos = tempVecPos[itVec];																		//load the postions that this state appears in obsData.
				loadStateString(1,statePos);																			//shift 1 place to the right to load the next state.

				if(hiddenStateIndex.find(stateTemp) != hiddenStateIndex.end()){											//Check the stateTemp exsists in our maps

					ithiddenStateIndex = hiddenStateIndex.find(stateTemp);												//find the iterator that points to the stateTemp.	

					tempVecData[ithiddenStateIndex->second] = tempVecData[ithiddenStateIndex->second] + 1;				//Add one to the correct location state current -> state next.
				}
			}

			for(int itVec = 0; itVec != tempVecData.size(); ++itVec){													// Go through every element in the loaded Data Vector and divide the next state counts by the current state total.

				if(tempVecData[itVec] != 0){																			// Ignore 0 values.
					tempVecData[itVec] = tempVecData[itVec] / totalEventCount;											// divide and store in the same location giving us a float percentage value.
				}
			}
																														//absorb skip end.
		}
		hiddenTrans[itCurrent->first].swap(tempVecData);																//Replace the vectors so it is ready to start again on the next iteration.
		hiddenDataPos[itHiddenDataPos->first].swap(tempVecPos);

	}

}

void Markov::makeObservationMap(){

	int tempVecLocation;
	vector<int> tempVecPos;
	vector<float> tempVecObsData;
	map<string, vector<float>>::iterator itObs;
	map<string, int>::iterator itObsEventIndex;
	float totalStateCount = 0;

	for(itHiddenDataPos = hiddenDataPos.begin(); itHiddenDataPos != hiddenDataPos.end(); ++itHiddenDataPos){

		hiddenDataPos[itHiddenDataPos->first].swap(tempVecPos);															// Swap out the vector containing the locations of the hidden state.
		itObs = obsPrbMap.find(itHiddenDataPos->first);																	// Find the location of the same hidden state key in the observation probability map.
		obsPrbMap[itObs->first].swap(tempVecObsData);																	// Swap out the vector at the hidden state location. 
		totalStateCount = tempVecPos.size();																			// How many times the hidden state appears in the origional data.

		for(int itVec = 0; itVec != tempVecPos.size(); ++itVec){														// This will give a total of how many of each event are seen at each hidden state.
					
			stateTemp = obsData[tempVecPos[itVec]];																		// Load the observation that matches the location of the hidden state.
			itObsEventIndex = obsEventIndex.find(stateTemp);															// Find the location of the event in the index map.
			tempVecLocation = itObsEventIndex->second;																	// Store the index int from the map in a temp variable. This means the event can be stored in the correct vector location.

			tempVecObsData[tempVecLocation] = tempVecObsData[tempVecLocation] + 1;										// Add one to the value of the int stored at that location in the obsPrbMap vector.

		}

		for(int itVec = 0; itVec != tempVecObsData.size(); ++itVec){													

			tempVecObsData[itVec] = tempVecObsData[itVec] / totalStateCount;											// Divide each value in the vector by how many times the hidden state appears.				

		}

		obsPrbMap[itObs->first].swap(tempVecObsData);																	// Replace the vector before iteration.
		hiddenDataPos[itHiddenDataPos->first].swap(tempVecPos);															// Replace the vector before iteration.
	}


}

bool Markov::fillAbsorbValues(vector<float>& vecData){												// Set absorb states to 1.

	bool vecDataEdit = false;
	ithiddenStateIndex = hiddenStateIndex.find(itCurrent->first);									//Find the state in the index and store the iterator location.
	for(int i = 0; i < failLocation.size(); ++i){													//Use the failLocation values which indicate the location of absorb states.

		if(failLocation[i] == ithiddenStateIndex->second){											//Check if the failLocation matches the state index.
			vecData[failLocation[i]] = 1;														    //If it matches then a 1 is put in that location in the hiddenTrans vector.
			vecDataEdit = true;																		//Set this to true so we can skip the rest of the calulation.
		}
	}
	return vecDataEdit;
}

void Markov::markovValueCheck(){																				//Check the total of each vector adds up to 1.

	vector<float> tempVecData;
	vector<float> initialPrb;

	cout << "\nInitial Probability \n";
	float vectorTotal = 0;

	for(int tempIt = 0; tempIt < initialPrb.size(); ++tempIt){

		vectorTotal = vectorTotal + initialPrb[tempIt];

	}
	cout << "\nTotal: " << vectorTotal;

	cout << "\n\nTransitions \n";

	for (itCurrent = hiddenTrans.begin(); itCurrent != hiddenTrans.end(); ++itCurrent){
		
		float vectorTotal = 0;
		hiddenTrans[itCurrent->first].swap(tempVecData);

		for(int itVec = 0; itVec != tempVecData.size(); ++itVec){

			vectorTotal = tempVecData[itVec] + vectorTotal;

		}
		
		cout << "\nState: " << itCurrent->first << " Total: " << vectorTotal;
		hiddenTrans[itCurrent->first].swap(tempVecData);
	}

	cout << "\n\nObservations \n";

	for (itCurrent = obsPrbMap.begin(); itCurrent != obsPrbMap.end(); ++itCurrent){
		
		float vectorTotal = 0;
		obsPrbMap[itCurrent->first].swap(tempVecData);

		for(int itVec = 0; itVec != tempVecData.size(); ++itVec){

			vectorTotal = tempVecData[itVec] + vectorTotal;

		}
		
		cout << "\nState: " << itCurrent->first << " Total: " << vectorTotal;
		obsPrbMap[itCurrent->first].swap(tempVecData);
	}

}

void Markov::calcBayes(){

	float initialPrbValue = 0;
	float obsPrbValue = 0;
	float secondInitialPrbValue = 0;
	float secondObsPrbValue = 0;
	float ResultPrb = 0;

	string hiddenStateUser;
	string obsUser;
	string secondHiddenState;

	vector<float> obsMapVec(0);																	// empty vector to swap with the vector stored in the map.
	

	cout << "\nPlease enter a "<< stateLengthMax <<" length state character: ";					// user enters a char string the size of the chosen state legnth. 
	cin >> hiddenStateUser;																		// store in temp string
	
	cout << "\nPlease enter the Observation Character: ";										// User enters the target Percentage they want the calculation reach.
	cin >> obsUser;																																								

	obtainBayesValues(initialPrbValue, obsPrbValue, hiddenStateUser, obsUser);					//Obtain data from maps to perform bayes calculation.

	if(hiddenStateUser == "m"){																	//Hard coded for this particular map needs refined for a more general solution.
		secondHiddenState = "s";
		obtainBayesValues(secondInitialPrbValue, secondObsPrbValue, secondHiddenState, obsUser);

	}else{
		secondHiddenState = "m";
		obtainBayesValues(secondInitialPrbValue, secondObsPrbValue, secondHiddenState, obsUser);

	}

	ResultPrb = (obsPrbValue*initialPrbValue) / ((obsPrbValue*initialPrbValue) + (secondObsPrbValue*secondInitialPrbValue)); //Bayes calc.																		
	
	cout << "\nThe probability of the state being:" << hiddenStateUser << " \nGiven observation: "<< obsUser << " \nResults from Bayes calc is = " << ResultPrb << "\n";

}	

void Markov::obtainBayesValues(float &initialVal, float &obsValue, string State, string Observation){

	map<string, float>::iterator tempitInitialState;
	map<string, vector<float>>::iterator tempitObsMap;
	map<string, int>::iterator tempitObsIndex;

	vector<float> obsMapVec(0);

	tempitInitialState = initialPrbMap.find(State);										//Locate the probability of the user entered state.
	tempitObsMap = obsPrbMap.find(State);												//Locate the probability vector of the user entered state.
	tempitObsIndex = obsEventIndex.find(Observation);									//Locate the vector of the user entered state.

	initialVal = initialPrbMap[tempitInitialState->first];								//Store Value in a float for calculation.
	obsMapVec = obsPrbMap[tempitObsMap->first];											//Store vector of observations for the user entered state in a vector.
	int itVec = tempitObsIndex->second;
	obsValue = obsMapVec[itVec];		

}

void Markov::calcFwdBkAlg(){
	
	map<string, int>::iterator tempitObsIndex;	

//	string userEventString;
	string obsEvent;
	int vecPos = 0;
	int fwdIterationValue = 0;

	cout << "\nPlease enter a sequence of observed events: ";	
	cin >> userEventString;

	for (int itVec = 0; itVec != userEventString.size(); ++itVec){										//Step forward through each observed event.

		obsEvent = userEventString[itVec];																//Extract a single event.
		tempitObsIndex = obsEventIndex.find(obsEvent);													//Find the correct vector location of the observed Event for the observation probability
		vecPos = tempitObsIndex->second;

		if(itVec == 0){																					//First iteration requires slightly different calculation.

			fwdMath(true, false, fwdIterationValue, vecPos, forwardPrbValues);

		}else{

			fwdMath(false, false, fwdIterationValue, vecPos, forwardPrbValues);

		}

		fwdIterationValue = itVec;																//Used to load previous calculations results from vector.
	
	}
	// Backwards Calc.

	fwdIterationValue = 0;

	for (int itVec = (userEventString.size()-1); itVec >= 0 ; itVec--){									//Step forward through each observed event.

		float normValue = 0;
		obsEvent = userEventString[itVec];																//Extract a single event.
		tempitObsIndex = obsEventIndex.find(obsEvent);													//Find the correct vector location of the observed Event for the observation probability
		vecPos = tempitObsIndex->second;

		if(itVec == (userEventString.size()-1)){														//First iteration requires slightly different calculation.

			fwdMath(true, true, fwdIterationValue, vecPos, backPrbValues);

		}else{

			fwdMath(false, false, fwdIterationValue, vecPos, backPrbValues);

		}

		fwdIterationValue =  (userEventString.size()-1) - itVec;																//Used to load previous calculations results from vector requires subtraction as the loop is going backwards and the result vector is getting created forwards.

	}

	calcSmoothResults();

}

void Markov::fwdMath(bool firstIteration, bool backCalcFlag, int fwdItValue, int obsLoc, map<string, vector<float>>& FwdBkPrbMap){

	float tempResultA = 0;	
	float normalisationValue = 0;
	int vecTransLoc = 0;


	vector<float> forwardVec(0);
	vector<float> obsMapVec(0);
	vector<float> transMapVec(0);
	vector<float> vecFwdResult(0);

	map<string, float>::iterator tempitInitialState;
	map<string, vector<float>>::iterator tempitObsMap;
	map<string, vector<float>>::iterator tempitForwardPrb;
	map<string, vector<float>>::iterator tempitFwdBkPrb;

	tempitInitialState = initialPrbMap.begin();

	for(tempitObsMap = obsPrbMap.begin(); tempitObsMap  != obsPrbMap.end(); ++tempitObsMap){



		if(firstIteration == true){

			switch (backCalcFlag){
				case false:
					tempResultA = tempitInitialState->second;
					tempitInitialState++;
				break;
				case true:
					tempResultA = 1;
				break;
			}

		}else{

			
			itCurrent = hiddenTrans.begin(); //
			transMapVec = hiddenTrans[itCurrent->first];	

			for(tempitForwardPrb = FwdBkPrbMap.begin(); tempitForwardPrb != FwdBkPrbMap.end(); ++tempitForwardPrb){

				forwardVec = FwdBkPrbMap[tempitForwardPrb->first];	

				tempResultA = transMapVec[vecTransLoc] * forwardVec[fwdItValue] + tempResultA;
					
				
				++itCurrent;
			}
	//
		}

		obsMapVec = obsPrbMap[tempitObsMap->first];
		tempResultA = tempResultA * obsMapVec[obsLoc];

		vecFwdResult.push_back(tempResultA);														//Store the result for all states in a vector.
		normalisationValue = normalisationValue + tempResultA;										//Total up the results to perform normalisation on the values once finished.
		tempResultA = 0;
		vecTransLoc++;																					//Move to the next Transition Vector Location
	}

	tempitFwdBkPrb = FwdBkPrbMap.begin();

		for(int itNormVec = 0; itNormVec != vecFwdResult.size(); ++itNormVec){					// Normalize values calculated

			vecFwdResult[itNormVec] = vecFwdResult[itNormVec] / normalisationValue;
		
			FwdBkPrbMap[tempitFwdBkPrb->first].push_back(vecFwdResult[itNormVec]);				 // Push the results into a map for storage.

			++tempitFwdBkPrb;
		}

}

void Markov::calcSmoothResults(){

	float normalisationValue = 0;
	float tempResultValue = 0;
	vector<float> tempResultVec(0);
	vector<float> fwdTempVec(0);
	vector<float> bkTempVec(0);
	map<string, vector<float>>::iterator itFwdPrb;
	map<string, vector<float>>::iterator itBkPrb;
	map<string, vector<float>>::iterator itSmooth;
	int fwdItVec = 0;
	int bkItVec = 0;
	itSmooth = smoothedValues.begin();

	for(itFwdPrb = forwardPrbValues.begin(), itBkPrb = backPrbValues.begin(); itFwdPrb != forwardPrbValues.end(); ++itFwdPrb, ++itBkPrb){

		fwdTempVec = forwardPrbValues[itFwdPrb->first];
		bkTempVec = backPrbValues[itBkPrb->first];

		for(fwdItVec = 0, bkItVec = (bkTempVec.size()-1); fwdItVec != fwdTempVec.size(); ++fwdItVec, bkItVec--){

			tempResultValue = fwdTempVec[fwdItVec] * bkTempVec[bkItVec];

			tempResultVec.push_back(tempResultValue);
			tempResultValue = 0;
		}
		smoothedValues[itSmooth->first].swap(tempResultVec);
		tempResultVec.clear();

		itSmooth++;
	}
	
	for( bkItVec = 0; bkItVec != fwdItVec; bkItVec++){

		normalisationValue = 0;
	
		for(itSmooth = smoothedValues.begin(); itSmooth != smoothedValues.end(); itSmooth++){

			tempResultVec = smoothedValues[itSmooth->first];

			normalisationValue = normalisationValue + tempResultVec[bkItVec];

		}


		for(itSmooth = smoothedValues.begin(); itSmooth != smoothedValues.end(); itSmooth++){

			smoothedValues[itSmooth->first].swap(tempResultVec);

			tempResultVec[bkItVec] = tempResultVec[bkItVec] / normalisationValue;

			smoothedValues[itSmooth->first].swap(tempResultVec);

		}



	}


}

#endif