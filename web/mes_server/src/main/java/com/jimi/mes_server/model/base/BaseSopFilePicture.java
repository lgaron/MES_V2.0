package com.jimi.mes_server.model.base;

import com.jfinal.plugin.activerecord.Model;
import com.jfinal.plugin.activerecord.IBean;

/**
 * Generated by JFinal, do not modify this file.
 */
@SuppressWarnings({"serial", "unchecked"})
public abstract class BaseSopFilePicture<M extends BaseSopFilePicture<M>> extends Model<M> implements IBean {

	public M setId(java.lang.Integer id) {
		set("id", id);
		return (M)this;
	}
	
	public java.lang.Integer getId() {
		return getInt("id");
	}

	public M setSopFileId(java.lang.Integer sopFileId) {
		set("sop_file_id", sopFileId);
		return (M)this;
	}
	
	public java.lang.Integer getSopFileId() {
		return getInt("sop_file_id");
	}

	public M setPictureNumber(java.lang.String pictureNumber) {
		set("picture_number", pictureNumber);
		return (M)this;
	}
	
	public java.lang.String getPictureNumber() {
		return getStr("picture_number");
	}

	public M setPictureName(java.lang.String pictureName) {
		set("picture_name", pictureName);
		return (M)this;
	}
	
	public java.lang.String getPictureName() {
		return getStr("picture_name");
	}

	public M setPicturePath(java.lang.String picturePath) {
		set("picture_path", picturePath);
		return (M)this;
	}
	
	public java.lang.String getPicturePath() {
		return getStr("picture_path");
	}

}