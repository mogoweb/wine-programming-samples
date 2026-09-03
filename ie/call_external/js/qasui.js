;
(function(window){
var DEBUG_MODE = false;
var external = window.external;
var isQASUI = false;
try{
	external.doCommand('qasui_test', {});
	isQASUI = true;
}catch(e){
}
if(!!window.QASUI) return;

function execCommand(cmd, args) {
	if(!DEBUG_MODE){
		if(isQASUI){
			if(!!external) {
//				alert(cmd);
				external.doCommand(cmd, args);
			}
		}
	}
}
var QASUI = {
	adjustwh: function(w, h){
		execCommand('adjustwh', {"width": w, "height":h});
	},
	setcenter: function(){
		execCommand('set_center', {});
	},
	setmodalresult: function(result){
		// mrNone     = 0;
  		// mrOk       = 1;
  		// mrCancel   = 2;
		execCommand('set_modalresult', {'result': result});
	},
	minwindow: function(){
		execCommand('min_wndow', {});
	},
	hidewindow: function(){
		execCommand('hide_window', {});
	},
	closewindow: function(){
		execCommand('close_window', {});
	},
	setuasession :function(sessionId){
		execCommand('set_ua_session', {'sessionId': sessionId});
	},
	getuasession: function(){
		var ret = {'sessionId': 'none'};
		execCommand('get_ua_session', ret);
		return ret.sessionId; 
	},
	setTallyTypeId: function(tallyTypeId){
		execCommand('set_tallyTypeId', {'tallyTypeId': tallyTypeId});
	},
	dragwindow: function(){
		execCommand('drag_window', {});
	},
	enableinfowindow: function(flag){
		execCommand('enable_info_window',{'flag': flag});
	},
	enablecountdownwindow: function(flag){
		execCommand('enable_countdown_window',{'flag': flag});
	},
	enableautohide: function(flag){
		execCommand('enable_autohide_window',{'flag': flag});
	},
	setDealInfoType: function(flag){
		execCommand('set_dealInfo_type',{'flag': flag});
	},
	notice: function(msg){
		execCommand('notice',{'message':msg});
	},
	prequalifypass: function() {
		execCommand('prequalify_pass', {});
	},
	prequalifyunpass: function() {
		execCommand('prequalify_unpass', {});
	},
	printNote: function(data, base64temp){
		var cmd = "printNote";
		var p = {count:0, data:[]};
		for(var name in data){
			p.data[p.count] = {name:name, value: data[name]};
			p.count++;
		}
		var args = {"noteinfo": p, "template": base64temp};
		execCommand(cmd, args);
	},
	eva_result:function(){
		execCommand('set_eva_result', {});
	},
	enableCountDownWithSec: function(flag){
		execCommand('enable_countdown_with_sec',{'flag': flag});
	},
	enableAutoPopupNoteDetail: function(flag){
		execCommand('enable_autopopup_notedetail',{'flag': flag});
	},
	enableEmgergencyStatusDetail: function(status){
		execCommand('set_liaoning_emgergency_status',{'status': status});
	},
	set_other_eva_way: function(evaWay){
		execCommand('set_other_eva_way', {'evaWay': evaWay});
	},
	setWaitCountShowType: function(flag){
		execCommand('set_wait_count_show_type', {'flag': flag});
	}
};
window.QASUI = QASUI;
})(window);
