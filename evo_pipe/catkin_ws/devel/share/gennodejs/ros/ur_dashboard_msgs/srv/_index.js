
"use strict";

let GetProgramState = require('./GetProgramState.js')
let IsProgramSaved = require('./IsProgramSaved.js')
let RawRequest = require('./RawRequest.js')
let AddToLog = require('./AddToLog.js')
let GetSafetyMode = require('./GetSafetyMode.js')
let Popup = require('./Popup.js')
let IsProgramRunning = require('./IsProgramRunning.js')
let Load = require('./Load.js')
let GetRobotMode = require('./GetRobotMode.js')
let IsInRemoteControl = require('./IsInRemoteControl.js')
let GetLoadedProgram = require('./GetLoadedProgram.js')

module.exports = {
  GetProgramState: GetProgramState,
  IsProgramSaved: IsProgramSaved,
  RawRequest: RawRequest,
  AddToLog: AddToLog,
  GetSafetyMode: GetSafetyMode,
  Popup: Popup,
  IsProgramRunning: IsProgramRunning,
  Load: Load,
  GetRobotMode: GetRobotMode,
  IsInRemoteControl: IsInRemoteControl,
  GetLoadedProgram: GetLoadedProgram,
};
