package com.jimi.mes_server.model;

import com.jfinal.plugin.activerecord.ActiveRecordPlugin;

/**
 * Generated by JFinal, do not modify this file.
 * <pre>
 * Example:
 * public void configPlugin(Plugins me) {
 *     ActiveRecordPlugin arp = new ActiveRecordPlugin(...);
 *     MappingKit.mapping(arp);
 *     me.add(arp);
 * }
 * </pre>
 */
public class MappingKit {
	
	public static void mapping(ActiveRecordPlugin arp) {
		arp.addMapping("action_log", "id", ActionLog.class);
		arp.addMapping("authority", "id", Authority.class);
		arp.addMapping("dashboard", "id", Dashboard.class);
		arp.addMapping("dashboard_for_customer", "id", DashboardForCustomer.class);
		arp.addMapping("DataRelativeSheet", "IMEI1", DataRelativeSheet.class);
		arp.addMapping("DataRelativeUnique", "DATA1", DataRelativeUnique.class);
		arp.addMapping("DataRelativeUpdate", "RelativeNum", DataRelativeUpdate.class);
		arp.addMapping("department", "id", Department.class);
		arp.addMapping("file_type", "id", FileType.class);
		arp.addMapping("Gps_AutoTest_AntiDup", "SN", GpsAutotestAntidup.class);
		arp.addMapping("Gps_AutoTest_Result", "Id", GpsAutotestResult.class);
		arp.addMapping("Gps_AutoTest_Result2", "Id", GpsAutotestResult2.class);
		arp.addMapping("Gps_AutoTest_Result3", "Id", GpsAutotestResult3.class);
		arp.addMapping("Gps_CartonBoxTwenty_Result", "Id", GpsCartonboxtwentyResult.class);
		arp.addMapping("Gps_CoupleTest_Result", "Id", GpsCoupletestResult.class);
		arp.addMapping("Gps_ManuCpParam", "ID", GpsManucpparam.class);
		arp.addMapping("Gps_ManuOrderParam", "Id", GpsManuorderparam.class);
		arp.addMapping("Gps_ManuPrintParam", "ID", GpsManuprintparam.class);
		arp.addMapping("Gps_ManuSimDataParam", "ID", GpsManusimdataparam.class);
		arp.addMapping("Gps_TestResult", "Id", GpsTestresult.class);
		arp.addMapping("line", "id", Line.class);
		arp.addMapping("line_computer", "id", LineComputer.class);
		// Composite Primary Key order: SN,SoftModel,Version
		arp.addMapping("LTestLogMessage", "SN,SoftModel,Version", LTestLogMessage.class);
		arp.addMapping("LUserAccount", "Id", LUserAccount.class);
		arp.addMapping("model_capacity", "id", ModelCapacity.class);
		arp.addMapping("model_capacity_status", "id", ModelCapacityStatus.class);
		arp.addMapping("order_file", "id", OrderFile.class);
		arp.addMapping("order_status", "id", OrderStatus.class);
		arp.addMapping("orders", "id", Orders.class);
		arp.addMapping("package_log", "id", PackageLog.class);
		arp.addMapping("process", "id", Process.class);
		arp.addMapping("process_group", "id", ProcessGroup.class);
		arp.addMapping("production_action_log", "id", ProductionActionLog.class);
		arp.addMapping("role", "id", Role.class);
		arp.addMapping("role_authority", "id", RoleAuthority.class);
		arp.addMapping("scheduling_plan", "id", SchedulingPlan.class);
		arp.addMapping("scheduling_plan_status", "id", SchedulingPlanStatus.class);
		arp.addMapping("sop_confirm_log", "id", SopConfirmLog.class);
		arp.addMapping("sop_count_log", "id", SopCountLog.class);
		arp.addMapping("sop_customer", "id", SopCustomer.class);
		arp.addMapping("sop_face_information", "id", SopFaceInformation.class);
		arp.addMapping("sop_factory", "id", SopFactory.class);
		arp.addMapping("sop_file", "id", SopFile.class);
		arp.addMapping("sop_file_history", "id", SopFileHistory.class);
		arp.addMapping("sop_file_picture", "id", SopFilePicture.class);
		arp.addMapping("sop_login_log", "id", SopLoginLog.class);
		arp.addMapping("sop_notice", "id", SopNotice.class);
		arp.addMapping("sop_notice_history", "id", SopNoticeHistory.class);
		arp.addMapping("sop_picture_history", "id", SopPictureHistory.class);
		arp.addMapping("sop_position_assignment", "id", SopPositionAssignment.class);
		arp.addMapping("sop_product_model", "id", SopProductModel.class);
		arp.addMapping("sop_series_model", "id", SopSeriesModel.class);
		arp.addMapping("sop_site", "id", SopSite.class);
		arp.addMapping("sop_site_display", "id", SopSiteDisplay.class);
		arp.addMapping("sop_workshop", "id", SopWorkshop.class);
		arp.addMapping("user_action_log", "id", UserActionLog.class);
		arp.addMapping("WebUserType", "TypeId", WebUserType.class);
		arp.addMapping("working_schedule", "id", WorkingSchedule.class);
	}
}

