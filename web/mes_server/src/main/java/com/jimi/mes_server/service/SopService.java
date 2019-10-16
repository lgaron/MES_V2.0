package com.jimi.mes_server.service;

import java.io.File;
import java.io.IOException;
import java.util.List;

import org.apache.commons.lang3.StringUtils;
import org.apache.poi.openxml4j.exceptions.InvalidFormatException;

import com.jfinal.kit.StrKit;
import com.jfinal.plugin.activerecord.Db;
import com.jfinal.plugin.activerecord.Page;
import com.jfinal.plugin.activerecord.Record;
import com.jfinal.plugin.activerecord.SqlPara;
import com.jfinal.upload.UploadFile;
import com.jimi.mes_server.entity.SopSQL;
import com.jimi.mes_server.exception.OperationException;
import com.jimi.mes_server.model.Line;
import com.jimi.mes_server.model.SopCustomer;
import com.jimi.mes_server.model.SopFactory;
import com.jimi.mes_server.model.SopProductModel;
import com.jimi.mes_server.model.SopSeriesModel;
import com.jimi.mes_server.model.SopSite;
import com.jimi.mes_server.model.SopWorkshop;
import com.jimi.mes_server.service.base.SelectService;
import com.jimi.mes_server.util.ExcelHelper;

public class SopService extends SelectService {

	public boolean addFactory(String factoryAlias, String abbreviation, String fullName) {
		SopFactory sopFactory = SopFactory.dao.findFirst(SopSQL.SELECT_FACTORY_ID_BY_COLUMN, factoryAlias, abbreviation, fullName);
		if (sopFactory != null) {
			throw new OperationException("数据项已存在,请重新输入");
		}
		sopFactory = new SopFactory();
		sopFactory.setAbbreviation(abbreviation).setFactoryAlias(factoryAlias).setFullName(fullName);
		return sopFactory.save();
	}


	public boolean deleteFactory(Integer id) {
		SopFactory sopFactory = SopFactory.dao.findById(id);
		if (sopFactory == null) {
			throw new OperationException("不存在当前工厂");
		}
		SopWorkshop sopWorkshop = SopWorkshop.dao.findFirst(SopSQL.SELECT_WORKSHOP_BY_FACTORY, id);
		if (sopWorkshop != null) {
			throw new OperationException("当前工厂被车间所引用，请先删除车间，再进行操作");
		}
		SopCustomer sopCustomer = SopCustomer.dao.findFirst(SopSQL.SELECT_CUSTOMER_BY_FACTORY, id);
		if (sopCustomer != null) {
			throw new OperationException("当前工厂被客户所引用，请先删除客户，再进行操作");
		}
		Line line = Line.dao.findFirst(SopSQL.SELECT_LINE_BY_FACTORY, id);
		if (line != null) {
			throw new OperationException("当前工厂被产线所引用，请先删除产线，再进行操作");
		}
		return sopFactory.delete();
	}


	public Page<Record> selectFactory(Integer pageNo, Integer pageSize, String ascBy, String descBy, String filter) {
		return select(SopSQL.SELECT_FACTORY, pageNo, pageSize, ascBy, descBy, filter);
	}


	public boolean editFactory(Integer id, String factoryAlias, String abbreviation, String fullName) {
		SopFactory sopFactory = SopFactory.dao.findById(id);
		if (sopFactory == null) {
			throw new OperationException("不存在当前工厂");
		}
		if (!StrKit.isBlank(factoryAlias)) {
			SopFactory temp = SopFactory.dao.findFirst(SopSQL.SELECT_FACTORY_ID_BY_COLUMN, factoryAlias, null, null);
			if (temp != null&&!temp.getId().equals(id)) {
				throw new OperationException("工厂别名已存在,请重新输入");
			}
			sopFactory.setFactoryAlias(factoryAlias);
		}
		if (!StrKit.isBlank(abbreviation)) {
			SopFactory temp = SopFactory.dao.findFirst(SopSQL.SELECT_FACTORY_ID_BY_COLUMN, null, abbreviation, null);
			if (temp != null&&!temp.getId().equals(id)) {
				throw new OperationException("工厂简称已存在,请重新输入");
			}
			sopFactory.setAbbreviation(abbreviation);
		}
		if (!StrKit.isBlank(fullName)) {
			SopFactory temp = SopFactory.dao.findFirst(SopSQL.SELECT_FACTORY_ID_BY_COLUMN, null, null, fullName);
			if (temp != null&&!temp.getId().equals(id)) {
				throw new OperationException("工厂全称已存在,请重新输入");
			}
			sopFactory.setFullName(fullName);
		}
		return sopFactory.update();
	}


	public boolean addWorkshop(Integer factoryId, String workshopNumber, String workshopName) {
		SopWorkshop sopWorkshop = SopWorkshop.dao.findFirst(SopSQL.SELECT_WORKSHOP_BY_COLUMN, workshopName, workshopNumber);
		if (sopWorkshop != null) {
			throw new OperationException("数据项已存在,请重新输入");
		}
		if (SopFactory.dao.findById(factoryId) == null) {
			throw new OperationException("不存在当前工厂");
		}
		sopWorkshop = new SopWorkshop();
		sopWorkshop.setFactoryId(factoryId).setWorkshopName(workshopName).setWorkshopNumber(workshopNumber);
		return sopWorkshop.save();
	}


	public boolean deleteWorkshop(Integer id) {
		SopWorkshop sopWorkshop = SopWorkshop.dao.findById(id);
		if (sopWorkshop == null) {
			throw new OperationException("不存在当前车间");
		}
		Line line = Line.dao.findFirst(SopSQL.SELECT_LINE_BY_WORKSHOP, id);
		if (line != null) {
			throw new OperationException("当前车间被产线所引用，请先删除产线，再进行操作");
		}
		return sopWorkshop.delete();
	}


	public Page<Record> selectWorkshop(Integer pageNo, Integer pageSize, Integer factoryId, String workshopNumber, String workshopName) {
		SqlPara sqlPara = new SqlPara();
		StringBuilder sql = new StringBuilder(SopSQL.SELECT_WORKSHOP_AND_FACTORY_ABBREVIATION);
		if (factoryId != null) {
			sql.append(" AND ").append("w.factory_id = ").append(factoryId);
		}
		if (!StringUtils.isBlank(workshopNumber)) {
			sql.append(" AND ").append("w.workshop_number like '%").append(workshopNumber).append("%'");
		}
		if (!StringUtils.isBlank(workshopName)) {
			sql.append(" AND ").append("w.workshop_name like '%").append(workshopName).append("%'");
		}
		sqlPara.setSql(sql.toString());
		return Db.paginate(pageNo, pageSize, sqlPara);
	}


	public boolean editWorkshop(Integer id, Integer factoryId, String workshopNumber, String workshopName) {
		SopWorkshop sopWorkshop = SopWorkshop.dao.findById(id);
		if (sopWorkshop == null) {
			throw new OperationException("不存在当前车间");
		}
		if (factoryId != null && !factoryId.equals(sopWorkshop.getFactoryId())) {
			if (SopFactory.dao.findById(factoryId) == null) {
				throw new OperationException("不存在当前工厂");
			}
			sopWorkshop.setFactoryId(factoryId);
		}
		if (!StringUtils.isBlank(workshopNumber)) {
			SopWorkshop temp = SopWorkshop.dao.findFirst(SopSQL.SELECT_WORKSHOP_BY_COLUMN, null, workshopNumber);
			if (temp != null&&!temp.getId().equals(id)) {
				throw new OperationException("车间编号已存在,请重新输入");
			}
			sopWorkshop.setWorkshopNumber(workshopNumber);
		}
		if (!StringUtils.isBlank(workshopName)) {
			SopWorkshop temp = SopWorkshop.dao.findFirst(SopSQL.SELECT_WORKSHOP_BY_COLUMN, workshopName, null);
			if (temp != null&&!temp.getId().equals(id)) {
				throw new OperationException("车间名称已存在,请重新输入");
			}
			sopWorkshop.setWorkshopName(workshopName);
		}
		return sopWorkshop.update();
	}


	public boolean addSite(String siteNumber, String siteName, Integer processOrder, Integer lineId, Integer playTimes, Integer switchInterval, String mac) {
		SopSite sopSite = SopSite.dao.findFirst(SopSQL.SELECT_SITE_BY_SITENUMBER, siteNumber);
		if (sopSite!=null) {
			throw new OperationException("站点编号已存在,请重新输入");
		}
		if (Line.dao.findById(lineId)==null) {
			throw new OperationException("当前产线不存在");
		}
		sopSite = new SopSite();
		if (playTimes!=null) {
			sopSite.setPlayTimes(playTimes);
		}
		sopSite.setLineId(lineId).setMac(mac).setProcessOrder(processOrder).setSiteName(siteName).setSiteNumber(siteNumber).setSwitchInterval(switchInterval);
		return sopSite.save();
	}


	public boolean deleteSite(Integer id) {
		SopSite sopSite = SopSite.dao.findById(id);
		if (sopSite==null) {
			throw new OperationException("当前站点不存在");
		}
		return sopSite.delete();
	}


	public Page<Record> selectWorkshop(Integer pageNo, Integer pageSize, String siteNumber, String siteName, Integer processOrder, Integer lineId) {
		SqlPara sqlPara = new SqlPara();
		StringBuilder sql = new StringBuilder(SopSQL.SELECT_SITE_JOIN_LINE);
		if (!StrKit.isBlank(siteName)) {
			sql.append(" and s.site_name like '%").append(siteName).append("%'");
		}
		if (!StrKit.isBlank(siteNumber)) {
			sql.append(" and s.site_number like '%").append(siteNumber).append("%'");
		}
		if (processOrder!=null) {
			sql.append(" and s.process_order = ").append(processOrder);
		}
		if (lineId!=null) {
			sql.append(" and s.line_id = ").append(lineId);
		}
		sqlPara.setSql(sql.toString());
		return Db.paginate(pageNo, pageSize, sqlPara);
	}


	public boolean editSite(Integer id, String siteNumber, String siteName, Integer processOrder, Integer lineId, Integer playTimes, Integer switchInterval, String mac) {
		SopSite sopSite = SopSite.dao.findById(id);
		if (sopSite==null) {
			throw new OperationException("当前站点不存在");
		}
		if (!StrKit.isBlank(siteNumber)) {
			SopSite temp = SopSite.dao.findFirst(SopSQL.SELECT_SITE_BY_SITENUMBER, siteNumber);
			if (temp!=null&&!temp.getId().equals(id)) {
				throw new OperationException("站点编号已存在,请重新输入");
			}
			sopSite.setSiteNumber(siteNumber);
		}
		if (!StrKit.isBlank(siteName)) {
			sopSite.setSiteName(siteName);
		}
		if (processOrder!=null) {
			sopSite.setProcessOrder(processOrder);
		}
		if (lineId!=null) {
			if (Line.dao.findById(lineId)==null) {
				throw new OperationException("当前产线不存在");
			}
			sopSite.setLineId(lineId);
		}
		if (playTimes!=null) {
			sopSite.setPlayTimes(playTimes);
		}
		if (switchInterval!=null) {
			sopSite.setSwitchInterval(switchInterval);
		}
		if (!StrKit.isBlank(mac)) {
			sopSite.setMac(mac);
		}
		return sopSite.update();
	}


	public boolean addCustomer(String customerNumber, String customerName, Integer factoryId) {
		SopCustomer sopCustomer = SopCustomer.dao.findFirst(SopSQL.SELECT_CUSTOMER_ID_BY_COLUMN, customerName,factoryId,customerNumber,factoryId);
		if (sopCustomer!=null) {
			throw new OperationException("同一工厂下已存在当前数据项，请重新输入");
		}
		if (SopFactory.dao.findById(factoryId)==null) {
			throw new OperationException("当前工厂不存在");
		}
		sopCustomer = new SopCustomer();
		sopCustomer.setFactoryId(factoryId).setCustomerName(customerName).setCustomerNumber(customerNumber);
		return sopCustomer.save();
	}


	public boolean deleteCustomer(Integer id) {
		SopCustomer sopCustomer = SopCustomer.dao.findById(id);
		if (sopCustomer==null) {
			throw new OperationException("当前客户不存在");
		}
		return sopCustomer.delete();
	}


	public Page<Record> selectCustomer(Integer pageNo, Integer pageSize, String customerNumber, String customerName, Integer factoryId) {
		SqlPara sqlPara = new SqlPara();
		StringBuilder sql = new StringBuilder(SopSQL.SELECT_CUSTOMER_JOIN_FACTORY);
		if (!StrKit.isBlank(customerName)) {
			sql.append(" AND c.customer_name LIKE '%").append(customerName).append("%'");
		}
		if (!StrKit.isBlank(customerNumber)) {
			sql.append(" AND c.customer_number LIKE '%").append(customerNumber).append("%'");
		}
		if (factoryId!=null) {
			sql.append(" AND c.factory_id = ").append(factoryId);
		}
		sqlPara.setSql(sql.toString());
		return Db.paginate(pageNo, pageSize, sqlPara);
	}


	public boolean editCustomer(Integer id, String customerNumber, String customerName, Integer factoryId) {
		SopCustomer sopCustomer = SopCustomer.dao.findById(id);
		if (sopCustomer==null) {
			throw new OperationException("当前客户不存在");
		}
		if (factoryId!=null&&!StringUtils.isAnyBlank(customerNumber,customerName)) {
			SopCustomer temp;
			if (!factoryId.equals(sopCustomer.getFactoryId())) {
				 temp = SopCustomer.dao.findFirst(SopSQL.SELECT_CUSTOMER_ID_BY_COLUMN, customerName,factoryId,customerNumber,factoryId);
			}else {
				 temp = SopCustomer.dao.findFirst(SopSQL.SELECT_CUSTOMER_ID_BY_COLUMN, customerName,sopCustomer.getFactoryId(),customerNumber,sopCustomer.getFactoryId());
			}
			if (temp!=null&&!id.equals(temp.getId())) {
				throw new OperationException("同一工厂下已存在当前数据项，请重新输入");
			}
			sopCustomer.setFactoryId(factoryId).setCustomerName(customerName).setCustomerNumber(customerNumber);
		}
		return sopCustomer.update();
	}


	public boolean addSeriesModel(String seriesModelName) {
		SopSeriesModel sopSeriesModel = SopSeriesModel.dao.findFirst(SopSQL.SELECT_SERIESMODEL_BY_SERIESMODELNAME, seriesModelName);
		if (sopSeriesModel!=null) {
			throw new OperationException("当前数据项已存在，请重新输入");
		}
		sopSeriesModel = new SopSeriesModel();
		sopSeriesModel.setSeriesModelName(seriesModelName);
		return sopSeriesModel.save();
	}


	public boolean deleteSeriesModel(Integer id) {
		SopSeriesModel sopSeriesModel = SopSeriesModel.dao.findById(id);
		if (sopSeriesModel==null) {
			throw new OperationException("当前系列机型不存在");
		}
		SopProductModel sopProductModel = SopProductModel.dao.findFirst(SopSQL.SELECT_PRODUCTMODEL_BY_SERIESMODEL, id);
		if (sopProductModel!=null) {
			throw new OperationException("当前系列机型已被产品机型所引用，请先删除产品机型，再进行操作");
		}
		return sopSeriesModel.delete();
	}


	public Page<Record> selectSeriesModel(Integer pageNo, Integer pageSize, String seriesModelName) {
		SqlPara sqlPara = new SqlPara();
		StringBuilder sql = new StringBuilder(SopSQL.SELECT_SERIESMODEL);
		if (!StrKit.isBlank(seriesModelName)) {
			sql.append(" WHERE series_model_name LIKE '%").append(seriesModelName).append("%'");
		}
		sqlPara.setSql(sql.toString());
		return Db.paginate(pageNo, pageSize, sqlPara);
	}


	public boolean editSeriesModel(Integer id, String seriesModelName) {
		SopSeriesModel sopSeriesModel = SopSeriesModel.dao.findById(id);
		if (sopSeriesModel==null) {
			throw new OperationException("当前系列机型不存在");
		}
		SopSeriesModel temp = SopSeriesModel.dao.findFirst(SopSQL.SELECT_SERIESMODEL_BY_SERIESMODELNAME, seriesModelName);
		if (temp!=null&&!id.equals(temp.getId())) {
			throw new OperationException("当前数据项已存在，请重新输入");
		}
		if (!StrKit.isBlank(seriesModelName)) {
			if (!seriesModelName.equals(sopSeriesModel.getSeriesModelName())) {
				sopSeriesModel.setSeriesModelName(seriesModelName);
				return sopSeriesModel.update();
			}
		}
		return true;
	}


	public boolean addProductModel(String productModelName, Integer seriesModelId) {
		if (SopSeriesModel.dao.findById(seriesModelId)==null) {
			throw new OperationException("当前系列机型不存在");
		}
		SopProductModel sopProductModel = SopProductModel.dao.findFirst(SopSQL.SELECT_PRODUCTMODEL_BY_NAME_ID, productModelName,seriesModelId);
		if (sopProductModel!=null) {
			throw new OperationException("当前系列机型此数据项已存在，请重新输入");
		}
		sopProductModel = new SopProductModel();
		sopProductModel.setProductModelName(productModelName).setSeriesModelId(seriesModelId);
		return sopProductModel.save();
	}


	public boolean deleteProductModel(Integer id) {
		SopProductModel sopProductModel = SopProductModel.dao.findById(id);
		if (sopProductModel==null) {
			throw new OperationException("当前产品机型不存在");
		}
		return sopProductModel.delete();
	}


	public Page<Record> selectProductModel(Integer pageNo, Integer pageSize, String productModelName, Integer seriesModelId) {
		SqlPara sqlPara = new SqlPara();
		StringBuilder sql = new StringBuilder(SopSQL.SELECT_PRODUCTMODEL_JOIN_SERIESMODEL);
		if (!StrKit.isBlank(productModelName)) {
			sql.append(" AND p.product_model_name like '%").append(productModelName).append("%'");
		}
		if (seriesModelId!=null) {
			sql.append(" AND p.series_model_id = ").append(seriesModelId);
		}
		sqlPara.setSql(sql.toString());
		return Db.paginate(pageNo, pageSize, sqlPara);
	}


	public boolean editProductModel(Integer id, String productModelName, Integer seriesModelId) {
		SopProductModel sopProductModel = SopProductModel.dao.findById(id);
		if (sopProductModel==null) {
			throw new OperationException("当前产品机型不存在");
		}
		if (SopSeriesModel.dao.findById(seriesModelId)==null) {
			throw new OperationException("当前系列机型不存在");
		}
		sopProductModel.setSeriesModelId(seriesModelId);
		if (!StrKit.isBlank(productModelName)) {
			SopProductModel temp = SopProductModel.dao.findFirst(SopSQL.SELECT_PRODUCTMODEL_BY_NAME_ID, productModelName,seriesModelId);
			if (temp!=null&&!id.equals(temp.getId())) {
				throw new OperationException("当前系列机型此数据项已存在，请重新输入");
			}
			sopProductModel.setProductModelName(productModelName);
		}
		return sopProductModel.update();
	}


	public boolean importFile(List<UploadFile> uploadFiles) {
			for (UploadFile uploadFile : uploadFiles) {
				saveFile(uploadFile);
				
			}
		
		return false;
	}
	
	private void saveFile(UploadFile uploadFile) {
		ExcelHelper excelHelper;
		try {
			excelHelper = ExcelHelper.from(uploadFile.getFile());
		} catch (Exception e) {
			throw new OperationException("读取文件出错，请确认文件信息");
		}
		excelHelper.switchSheet(1);
		String fileName = excelHelper.getString(10, 2);
		String fileNumber = excelHelper.getString(11, 2);
		String version = excelHelper.getString(12, 2);
		excelHelper.switchSheet(2);
		String customer = excelHelper.getString(2, 2);
		String productModel = excelHelper.getString(2, 6);
		String seriesModel = excelHelper.getString(2, 11);

	}
	
	private void saveFileInfo(UploadFile uploadFile) {
		
	}

}