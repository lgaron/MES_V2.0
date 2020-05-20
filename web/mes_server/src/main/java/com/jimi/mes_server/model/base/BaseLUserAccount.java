package com.jimi.mes_server.model.base;

import com.jfinal.plugin.activerecord.Model;
import com.jfinal.plugin.activerecord.IBean;

/**
 * Generated by JFinal, do not modify this file.
 */
@SuppressWarnings({"serial", "unchecked"})
public abstract class BaseLUserAccount<M extends BaseLUserAccount<M>> extends Model<M> implements IBean {

	public M setName(java.lang.String Name) {
		set("Name", Name);
		return (M)this;
	}
	
	public java.lang.String getName() {
		return getStr("Name");
	}

	public M setPassword(java.lang.String Password) {
		set("Password", Password);
		return (M)this;
	}
	
	public java.lang.String getPassword() {
		return getStr("Password");
	}

	public M setUserType(java.lang.String UserType) {
		set("UserType", UserType);
		return (M)this;
	}
	
	public java.lang.String getUserType() {
		return getStr("UserType");
	}

	public M setMaskFromV2(byte[] MaskFromV2) {
		set("_MASK_FROM_V2", MaskFromV2);
		return (M)this;
	}
	
	public byte[] getMaskFromV2() {
		return get("_MASK_FROM_V2");
	}

	public M setId(java.lang.Integer Id) {
		set("Id", Id);
		return (M)this;
	}
	
	public java.lang.Integer getId() {
		return getInt("Id");
	}

	public M setLoginTime(java.util.Date LoginTime) {
		set("LoginTime", LoginTime);
		return (M)this;
	}
	
	public java.util.Date getLoginTime() {
		return get("LoginTime");
	}

	public M setInService(java.lang.Boolean InService) {
		set("InService", InService);
		return (M)this;
	}
	
	public java.lang.Boolean getInService() {
		return get("InService");
	}

	public M setLoginStatus(java.lang.Integer LoginStatus) {
		set("LoginStatus", LoginStatus);
		return (M)this;
	}
	
	public java.lang.Integer getLoginStatus() {
		return getInt("LoginStatus");
	}

	public M setDeletePermission(java.lang.String DeletePermission) {
		set("DeletePermission", DeletePermission);
		return (M)this;
	}
	
	public java.lang.String getDeletePermission() {
		return getStr("DeletePermission");
	}

	public M setUserDes(java.lang.String UserDes) {
		set("UserDes", UserDes);
		return (M)this;
	}
	
	public java.lang.String getUserDes() {
		return getStr("UserDes");
	}

	public M setWebUserType(java.lang.Integer WebUserType) {
		set("WebUserType", WebUserType);
		return (M)this;
	}
	
	public java.lang.Integer getWebUserType() {
		return getInt("WebUserType");
	}

	public M setLineName(java.lang.String LineName) {
		set("LineName", LineName);
		return (M)this;
	}
	
	public java.lang.String getLineName() {
		return getStr("LineName");
	}

	public M setEmployeeType(java.lang.String EmployeeType) {
		set("EmployeeType", EmployeeType);
		return (M)this;
	}
	
	public java.lang.String getEmployeeType() {
		return getStr("EmployeeType");
	}

	public M setMainProcess(java.lang.String MainProcess) {
		set("MainProcess", MainProcess);
		return (M)this;
	}
	
	public java.lang.String getMainProcess() {
		return getStr("MainProcess");
	}

	public M setProficiency(java.lang.String Proficiency) {
		set("Proficiency", Proficiency);
		return (M)this;
	}
	
	public java.lang.String getProficiency() {
		return getStr("Proficiency");
	}

	public M setOtherProcess(java.lang.String OtherProcess) {
		set("OtherProcess", OtherProcess);
		return (M)this;
	}
	
	public java.lang.String getOtherProcess() {
		return getStr("OtherProcess");
	}

	public M setIsOnline(java.lang.Boolean IsOnline) {
		set("IsOnline", IsOnline);
		return (M)this;
	}
	
	public java.lang.Boolean getIsOnline() {
		return get("IsOnline");
	}

	public M setRole(java.lang.Integer Role) {
		set("Role", Role);
		return (M)this;
	}
	
	public java.lang.Integer getRole() {
		return getInt("Role");
	}

	public M setIPAddress(java.lang.String IPAddress) {
		set("IPAddress", IPAddress);
		return (M)this;
	}
	
	public java.lang.String getIPAddress() {
		return getStr("IPAddress");
	}

	public M setDelete(java.lang.Boolean Delete) {
		set("Delete", Delete);
		return (M)this;
	}
	
	public java.lang.Boolean getDelete() {
		return get("Delete");
	}

}
