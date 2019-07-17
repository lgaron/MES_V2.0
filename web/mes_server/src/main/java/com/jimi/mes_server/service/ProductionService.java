package com.jimi.mes_server.service;

import java.io.File;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.apache.commons.io.FileUtils;
import org.apache.commons.lang3.StringUtils;
import org.apache.commons.lang3.time.DateUtils;

import com.jfinal.aop.Enhancer;
import com.jfinal.kit.PropKit;
import com.jfinal.kit.StrKit;
import com.jfinal.plugin.activerecord.Db;
import com.jfinal.plugin.activerecord.Page;
import com.jfinal.plugin.activerecord.Record;
import com.jfinal.plugin.activerecord.SqlPara;
import com.jfinal.upload.UploadFile;
import com.jimi.mes_server.entity.Constant;
import com.jimi.mes_server.entity.OrderItem;
import com.jimi.mes_server.entity.SQL;
import com.jimi.mes_server.entity.vo.LUserAccountVO;
import com.jimi.mes_server.entity.vo.OrderVO;
import com.jimi.mes_server.exception.OperationException;
import com.jimi.mes_server.exception.ParameterException;
import com.jimi.mes_server.model.LUserAccount;
import com.jimi.mes_server.model.Line;
import com.jimi.mes_server.model.ModelCapacity;
import com.jimi.mes_server.model.OrderFile;
import com.jimi.mes_server.model.Orders;
import com.jimi.mes_server.model.Process;
import com.jimi.mes_server.model.ProcessGroup;
import com.jimi.mes_server.model.SchedulingPlan;
import com.jimi.mes_server.service.base.SelectService;
import com.jimi.mes_server.util.ExcelHelper;
import com.jimi.mes_server.util.ResultUtil;

public class ProductionService {

	private static SelectService daoService = Enhancer.enhance(SelectService.class);

	public boolean addProcessGroup(String groupNo, String groupName, String groupRemark) {
		if (ProcessGroup.dao.findFirst(SQL.SELECT_PROCESSGROUP_BY_GROUPNO, groupNo) != null) {
			throw new OperationException("工序组编号已存在");
		}
		if (ProcessGroup.dao.findFirst(SQL.SELECT_PROCESSGROUP_BY_GROUPNAME, groupName) != null) {
			throw new OperationException("工序组名称已存在");
		}
		ProcessGroup processGroup = new ProcessGroup();
		processGroup.setGroupNo(groupNo).setGroupName(groupName);
		if (!StrKit.isBlank(groupRemark)) {
			processGroup.setGroupRemark(groupRemark);
		}
		return processGroup.save();
	}

	public boolean deleteProcessGroup(Integer id) {
		ProcessGroup processGroup = ProcessGroup.dao.findById(id);
		if (processGroup == null) {
			throw new OperationException("删除失败，项目组不存在");
		}
		return processGroup.delete();
	}

	public boolean editProcessGroup(Integer id, String groupNo, String groupName, String groupRemark) {
		ProcessGroup processGroup = ProcessGroup.dao.findById(id);
		if (processGroup == null) {
			throw new OperationException("修改失败，项目组不存在");
		}

		if (!StrKit.isBlank(groupNo)) {
			if (ProcessGroup.dao.findFirst(SQL.SELECT_PROCESSGROUP_BY_GROUPNO, groupNo) != null) {
				throw new OperationException("工序组编号已存在");
			}
			processGroup.setGroupNo(groupNo);
		}
		if (!StrKit.isBlank(groupName)) {
			if (ProcessGroup.dao.findFirst(SQL.SELECT_PROCESSGROUP_BY_GROUPNAME, groupName) != null) {
				throw new OperationException("工序组名称已存在");
			}
			processGroup.setGroupName(groupName);
		}
		if (groupRemark != null) {
			processGroup.setGroupRemark(groupRemark);
		}
		return processGroup.update();
	}

	public boolean addLine(String lineNo, String lineName, String lineRemark, Integer lineDirector,
			Integer processGroup) {
		if (Line.dao.findFirst(SQL.SELECT_LINE_BY_LINENO, lineNo) != null) {
			throw new OperationException("产线线别已存在");
		}
		if (Line.dao.findFirst(SQL.SELECT_LINE_BY_LINENAME, lineName) != null) {
			throw new OperationException("产线名称已存在");
		}
		Line line = new Line();
		line.setLineNo(lineNo).setLineName(lineName);
		if (!StrKit.isBlank(lineRemark)) {
			line.setLineRemark(lineRemark);
		}
		if (lineDirector != null) {
			checkUserById(lineDirector);
			line.setLineDirector(lineDirector);
		}
		if (processGroup != null) {
			if (ProcessGroup.dao.findById(processGroup) == null) {
				throw new OperationException("工序组不存在");
			}
			line.setLineDirector(processGroup);
		}
		return line.save();
	}

	public boolean deleteLine(Integer id) {
		Line line = Line.dao.findById(id);
		if (line == null) {
			throw new OperationException("删除失败，产线不存在");
		}
		return line.delete();
	}

	public Page<Record> selectLine(String lineNo, String lineName, Integer processGroup) {
		StringBuilder filter = new StringBuilder();
		if (!StrKit.isBlank(lineNo)) {
			filter.append("line_no like '%" + lineNo + " %' and");
		}
		if (!StrKit.isBlank(lineName)) {
			filter.append(" line_name like '%" + lineName + " %' and");
		}
		if (processGroup != null) {

			if (ProcessGroup.dao.findById(processGroup) == null) {
				throw new OperationException("工序组不存在");
			}
			filter.append(" process_group = " + processGroup);
		}
		if (StringUtils.endsWith(filter, "and")) {
			filter.delete(filter.lastIndexOf("and"), filter.length());
		}
		SqlPara sqlPara = new SqlPara();
		if (filter.length() == 0) {
			sqlPara.setSql(SQL.SELECT_LINE_JOIN_PROCESSGROUP);
		} else {
			sqlPara.setSql(SQL.SELECT_LINE_JOIN_PROCESSGROUP + " where " + filter);
		}

		return Db.paginate(Constant.DEFAULT_PAGE_NUM, Constant.DEFAULT_PAGE_SIZE, sqlPara);
	}

	public boolean editLine(Integer id, String lineNo, String lineName, String lineRemark, Integer lineDirector,
			Integer processGroup) {
		Line line = Line.dao.findById(id);
		if (line == null) {
			throw new OperationException("修改失败，产线不存在");
		}

		if (!StrKit.isBlank(lineNo)) {
			if (ProcessGroup.dao.findFirst(SQL.SELECT_LINE_BY_LINENO, lineNo) != null) {
				throw new OperationException("产线线别已存在");
			}
			line.setLineNo(lineNo);
		}
		if (!StrKit.isBlank(lineName)) {
			if (ProcessGroup.dao.findFirst(SQL.SELECT_LINE_BY_LINENAME, lineName) != null) {
				throw new OperationException("产线名称已存在");
			}
			line.setLineName(lineName);
		}
		if (lineRemark != null) {
			line.setLineRemark(lineRemark);
		}
		if (lineDirector != null) {
			checkUserById(lineDirector);
			line.setLineDirector(lineDirector);
		}
		if (processGroup != null) {
			if (ProcessGroup.dao.findById(processGroup) == null) {
				throw new OperationException("工序组不存在");
			}
			line.setProcessGroup(processGroup);
		}

		return line.update();
	}

	public boolean addProcess(String processNo, String processName, String processRemark, Integer processGroup) {
		if (Process.dao.findFirst(SQL.SELECT_PROCESS_BY_PROCESSNO, processNo) != null) {
			throw new OperationException("工序编号已存在");
		}
		if (Process.dao.findFirst(SQL.SELECT_PROCESS_BY_PROCESSNAME, processName) != null) {
			throw new OperationException("工序名称已存在");
		}
		Process process = new Process();
		process.setProcessNo(processNo).setProcessName(processName);
		if (!StrKit.isBlank(processRemark)) {
			process.setProcessRemark(processRemark);
		}
		if (processGroup != null) {
			if (ProcessGroup.dao.findById(processGroup) == null) {
				throw new OperationException("工序组不存在");
			}
			process.setProcessGroup(processGroup);
		}
		return process.save();
	}

	public boolean deleteProcess(Integer id) {
		Process process = Process.dao.findById(id);
		if (process == null) {
			throw new OperationException("删除失败，工序不存在");
		}
		return process.delete();
	}

	public Page<Record> selectProcess(String processNo, String processName, Integer processGroup) {
		StringBuilder filter = new StringBuilder();
		if (!StrKit.isBlank(processNo)) {
			filter.append("process_no like '%" + processNo + " %' and");
		}
		if (!StrKit.isBlank(processName)) {
			filter.append(" process_name like '%" + processName + " %' and");
		}
		if (processGroup != null) {

			if (ProcessGroup.dao.findById(processGroup) == null) {
				throw new OperationException("工序组不存在");
			}
			filter.append(" process_group = " + processGroup);
		}
		if (StringUtils.endsWith(filter, "and")) {
			filter.delete(filter.lastIndexOf("and"), filter.length());
		}
		SqlPara sqlPara = new SqlPara();
		if (filter.length() == 0) {
			sqlPara.setSql(SQL.SELECT_PROCESS_JOIN_PROCESSGROUP);
		} else {
			sqlPara.setSql(SQL.SELECT_PROCESS_JOIN_PROCESSGROUP + " where " + filter);
		}

		return Db.paginate(Constant.DEFAULT_PAGE_NUM, Constant.DEFAULT_PAGE_SIZE, sqlPara);
	}

	public boolean editProcess(Integer id, String processNo, String processName, String processRemark,
			Integer processGroup) {
		Process process = Process.dao.findById(id);
		if (process == null) {
			throw new OperationException("修改失败，工序不存在");
		}

		if (!StrKit.isBlank(processNo)) {
			if (Process.dao.findFirst(SQL.SELECT_PROCESS_BY_PROCESSNO, processNo) != null) {
				throw new OperationException("工序编号已存在");
			}
			process.setProcessNo(processNo);
		}
		if (!StrKit.isBlank(processName)) {
			if (Process.dao.findFirst(SQL.SELECT_PROCESS_BY_PROCESSNAME, processName) != null) {
				throw new OperationException("工序名称已存在");
			}
			process.setProcessName(processName);
		}
		if (processRemark != null) {
			process.setProcessRemark(processRemark);
		}
		if (processGroup != null) {
			if (ProcessGroup.dao.findById(processGroup) == null) {
				throw new OperationException("工序组不存在");
			}
			process.setProcessGroup(processGroup);
		}

		return process.update();
	}

	public boolean addOrder(Orders order) {
		Orders temp = Orders.dao.findFirst(SQL.SELECT_ORDER_BY_ZHIDAN, order.getZhidan());
		if (temp != null) {
			throw new OperationException("订单号已存在");
		}
		order.setOrdersStatus(Constant.UNSCHEDULED_ORDERSTATUS);
		return order.save();
	}

	public boolean deleteOrder(Integer id, String deleteReason) {
		Orders order = Orders.dao.findById(id);
		if (order == null) {
			throw new OperationException("删除失败，订单不存在");
		}
		if (!Constant.UNSCHEDULED_ORDERSTATUS.equals(order.getOrdersStatus())) {
			throw new OperationException("删除失败，只能删除未排产订单");
		}
		order.setDeleteReason(deleteReason).setOrdersStatus(Constant.DELETED_ORDERSTATUS);
		return order.update();
	}

	public boolean editOrder(Orders order) {
		Orders temp = Orders.dao.findById(order.getId());
		if (temp == null) {
			throw new OperationException("订单不存在");
		}
		return order.update();
	}

	public String importOrder(File file) {
		String resultString = "导入成功";
		ExcelHelper helper;
		int indexOfOrderItem = 2;
		List<Orders> orders = new ArrayList<>();
		try {
			helper = ExcelHelper.from(file);
			List<OrderItem> orderItems = helper.unfill(OrderItem.class, 0);
			if (orderItems == null) {
				throw new OperationException("表格无有效数据或者表格格式不正确！");
			}
			for (OrderItem orderItem : orderItems) {
				String zhidan = orderItem.getZhidan();
				String softModel = orderItem.getSoftModel();
				if (!StringUtils.isAnyBlank(zhidan, softModel) && orderItem.getQuantity() != null) {
					Orders temp = Orders.dao.findFirst(SQL.SELECT_ORDER_BY_ZHIDAN, orderItem.getZhidan());
					if (temp != null) {
						resultString = "导入失败，表格第" + indexOfOrderItem + "行的订单号已存在！";
						return resultString;
					}
					Orders order = new Orders();
					order.setZhidan(zhidan).setSoftModel(softModel);
					order.setCustomerName(orderItem.getCustomerName()).setCustomerNumber(orderItem.getCustomerNumber());
					order.setAlias(orderItem.getAlias()).setProductNo(orderItem.getProductNo())
							.setCreateTime(orderItem.getCreateTime());
					order.setQuantity(orderItem.getQuantity()).setDeliveryDate(orderItem.getDeliveryDate())
							.setRemark(orderItem.getRemark());
					orders.add(order);
				}
				indexOfOrderItem++;
			}
			for (Orders order : orders) {
				order.setOrdersStatus(Constant.UNSCHEDULED_ORDERSTATUS).save();
			}
		} catch (Exception e) {
			e.printStackTrace();
			throw new OperationException("文件解析出错，请检查文件内容和格式");
		} finally {
			file.delete();
		}
		return resultString;
	}

	public boolean importOrderTable(List<UploadFile> uploadFiles, Integer type, Integer orderId) {
		// TODO Auto-generated method stub
		OrderFile orderFile = new OrderFile();
		switch (type) {
		case 0:
			orderFile.setFileType(Constant.INFORMATION_FILETYPE);
			break;
		case 1:
			orderFile.setFileType(Constant.BOM_FILETYPE);
			break;
		case 2:
			orderFile.setFileType(Constant.SOP_FILETYPE);
			break;

		default:
			throw new OperationException("无法识别的类型");
		}
		Orders order = Orders.dao.findById(orderId);
		if (order == null) {
			throw new OperationException("订单不存在");
		}
		for (UploadFile uploadFile : uploadFiles) {
			File tempFile = uploadFile.getFile();
			String fileName = uploadFile.getFileName();
			// 防止文件名中携带&nbsp的空格，导致前端输入文件名查不到数据
			try {
				fileName = URLEncoder.encode(fileName, "utf-8");
				fileName = fileName.replaceAll("%C2%A0", "%20");
				fileName = URLDecoder.decode(fileName, "utf-8");
			} catch (UnsupportedEncodingException e) {
				e.printStackTrace();
			}
			File dir = new File(Constant.FILE_TABLE_PATH);
			if (!dir.exists()) {
				dir.mkdirs();
			}
			File file = new File(Constant.FILE_TABLE_PATH + fileName);
			if (file.exists()) {
				file.delete();
			}
			try {
				FileUtils.moveFile(tempFile, file);
			} catch (IOException e) {
				throw new OperationException("文件保存失败");
			}

			orderFile.setFileName(fileName).setPath(file.getAbsolutePath()).setOrders(orderId).save();
			orderFile.remove("id");

		}
		return true;
	}

	public List<OrderFile> selectOrderTable(Integer type, Integer id) {
		Orders order = Orders.dao.findById(id);
		if (order == null) {
			throw new OperationException("订单不存在");
		}
		if (type == null) {
			return OrderFile.dao.find(SQL.SELECT_ORDERFILE_BY_ORDER, id);
		} else {
			return OrderFile.dao.find(SQL.SELECT_ORDERFILE_BY_ORDER_FILETYPE, id, type);
		}
	}

	public File downloadOrderTable(Integer id) {

		OrderFile orderFile = OrderFile.dao.findById(id);
		if (orderFile == null) {
			throw new OperationException("文件不存在");
		}
		File file = new File(orderFile.getPath());
		return file;
	}

	public boolean addCapacity(String softModel, String customerModel, Integer process, Integer processGroup,
			Integer processPeopleQuantity, Integer capacity, String remark) {
		ModelCapacity modelCapacity = ModelCapacity.dao.findFirst(SQL.SELECT_MODELCAPACITY_BY_MODEL_PROCESS, softModel,
				process, processGroup);
		if (modelCapacity != null) {
			throw new OperationException("机型产能已存在");
		}
		modelCapacity = new ModelCapacity();
		if (remark != null) {
			modelCapacity.setRemark(remark);
		}
		modelCapacity.setSoftModel(softModel).setCustomerModel(customerModel).setProcess(process)
				.setProcessGroup(processGroup);
		modelCapacity.setProcessPeopleQuantity(processPeopleQuantity).setCapacity(capacity).save();
		return modelCapacity.setPosition(modelCapacity.getId()).update();
	}

	public boolean deleteCapacity(Integer id) {
		ModelCapacity modelCapacity = ModelCapacity.dao.findById(id);
		if (modelCapacity == null) {
			throw new OperationException("删除失败，机型产能不存在");
		}
		return modelCapacity.delete();
	}

	public Page<Record> select(String tableName, Integer pageNo, Integer pageSize, String ascBy, String descBy,
			String filter) {
		return daoService.select(tableName, pageNo, pageSize, ascBy, descBy, filter, null);
	}

	public Page<Record> select(String tableName, String filter) {
		return daoService.select(tableName, 1, PropKit.use("properties.ini").getInt("defaultPageSize"), null, null,
				filter, null);
	}

	public Record getPlanGannt(Integer id) {
		Record record = Db.findFirst(SQL.SELECT_PLAN_GANT_INFORMATION, id);
		if (record == null) {
			throw new OperationException("查询失败，计划不存在");
		}
		record.set("interval", getDateIntervalDays(record.getStr("startTime"), record.getStr("endTime")));

		return record;
	}

	private void checkUserById(Integer id) {
		LUserAccount user = LUserAccount.dao.findById(id);
		if (user == null) {
			throw new OperationException("产线负责人不存在");
		}
		if (!user.getInService()) {
			throw new OperationException("产线负责人未启用");
		}
	}

	private long getDateIntervalDays(String startDate, String endDate) {
		long daysBetween = 0;
		SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd");

		// 解析String为Date

		try {
			Date start = format.parse(startDate);
			Date end = format.parse(endDate);
			daysBetween = (end.getTime() - start.getTime()) / (3600 * 24 * 1000);
		} catch (ParseException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		return daysBetween;
	}

	public List<OrderVO> selectUnscheduledPlan(Integer type) {
		List<OrderVO> orderVOs = new ArrayList<>();
		List<Record> records = new ArrayList<>();
		switch (type) {
		case 0:
			List<Orders> orders = Orders.dao.find(SQL.SELECT_ORDER_BY_STATUS, Constant.UNSCHEDULED_ORDERSTATUS);
			for (Orders order : orders) {
				Record record = Db.findFirst(SQL.SELECT_PEOPLE_CAPACITY_BY_SOFTMODEL_PROCESSGROUP,
						Constant.ASSEMBLING_PROCESS_GROUP, "%" + order.getSoftModel() + "%");
				OrderVO orderVO = new OrderVO(order, order.getQuantity(),
						record.getInt("capacity") / record.getInt("people"));
				orderVOs.add(orderVO);
			}
			return orderVOs;
		case 1:
			records = Db.find(SQL.SELECT_SCHEDULED_ORDER_QUANTITY, Constant.ASSEMBLING_PROCESS_GROUP);
			break;
		case 2:
			records = Db.find(SQL.SELECT_SCHEDULED_ORDER_QUANTITY, Constant.TESTING_PROCESS_GROUP);
			break;
		default:
			break;
		}
		for (Record record : records) {
			Integer orderId = record.getInt("orders");
			Integer scheduledQuantity = record.getInt("scheduled_quantity");
			Orders order = Orders.dao.findById(orderId);
			Record peopleCapacityRecord = new Record();
			if (type == 1) {
				peopleCapacityRecord = Db.findFirst(SQL.SELECT_PEOPLE_CAPACITY_BY_SOFTMODEL_PROCESSGROUP,
						Constant.TESTING_PROCESS_GROUP, "%" + order.getSoftModel() + "%");

			} else if (type == 2) {
				peopleCapacityRecord = Db.findFirst(SQL.SELECT_PEOPLE_CAPACITY_BY_SOFTMODEL_PROCESSGROUP,
						Constant.PACKING_PROCESS_GROUP, "%" + order.getSoftModel() + "%");
			}
			OrderVO orderVO = new OrderVO(order, order.getQuantity() - scheduledQuantity,
					peopleCapacityRecord.getInt("capacity") / peopleCapacityRecord.getInt("people"));
			orderVOs.add(orderVO);
		}
		return orderVOs;
	}

	public boolean editPlanStatus(Integer id, Integer type, LUserAccountVO userVO) {
		SchedulingPlan schedulingPlan = SchedulingPlan.dao.findById(id);
		if (schedulingPlan == null) {
			throw new OperationException("排产计划不存在");
		}
		switch (type) {
		case 0:
			schedulingPlan.setSchedulingPlanStatus(Constant.SCHEDULED_PLANSTATUS);
			break;
		case 1:
			schedulingPlan.setSchedulingPlanStatus(Constant.WORKING_PLANSTATUS);
			break;
		case 2:
			schedulingPlan.setSchedulingPlanStatus(Constant.COMPLETED_PLANSTATUS);
			break;
		case 3:
			schedulingPlan.setSchedulingPlanStatus(Constant.WAIT_NOTIFICATION_PLANSTATUS);
			break;
		default:
			break;
		}

		return schedulingPlan.update();
	}

	public boolean deletePlan(Integer id) {
		SchedulingPlan schedulingPlan = SchedulingPlan.dao.findById(id);
		if (schedulingPlan == null) {
			throw new OperationException("排产计划不存在");
		}

		return schedulingPlan.delete();
	}

	public boolean addPlan(Integer order, String remark, Integer schedulingQuantity, Integer line, Integer processGroup,
			Integer capacity) {
		if (Line.dao.findById(line) == null) {
			throw new OperationException("添加排产计划失败，产线不存在");
		}
		Orders orderRecord = Orders.dao.findById(order);
		if (orderRecord == null) {
			throw new OperationException("订单不存在");
		}
		SchedulingPlan schedulingPlan = new SchedulingPlan();
		if (remark != null) {
			schedulingPlan.setRemark(remark);
		}
		schedulingPlan.setProcessGroup(processGroup).setLine(line).setSchedulingQuantity(schedulingQuantity)
				.setOrders(order).setLineChangeTime(Constant.DEFAULT_LINE_CHANGE_TIME).setCapacity(capacity)
				.setSchedulingPlanStatus(Constant.SCHEDULED_PLANSTATUS);

		return schedulingPlan.save();
	}

	public boolean editPlan(Integer id, Boolean isUrgent, String remark, Integer schedulingQuantity, Integer line,
			Date planStartTime, Date planCompleteTime, String lineChangeTime, Integer capacity, Boolean isCompleted,
			Integer producedQuantity, String remainingReason, String productionPlanningNumber) {
		SchedulingPlan schedulingPlan = SchedulingPlan.dao.findById(id);
		if (schedulingPlan == null) {
			throw new OperationException("排产计划不存在");
		}
		schedulingPlan.setIsUrgent(isUrgent).setLine(line).setPlanStartTime(planStartTime)
				.setPlanCompleteTime(planCompleteTime);
		if (remark != null) {
			schedulingPlan.setRemark(remark);
		}
		if (!StrKit.isBlank(lineChangeTime) && lineChangeTime.length() < 8) {
			schedulingPlan.setLineChangeTime(lineChangeTime);
		} else {
			throw new ParameterException("转线时间的长度过长");
		}
		if (capacity != null) {
			schedulingPlan.setCapacity(capacity);
		}
		if (isCompleted) {
			schedulingPlan.setSchedulingPlanStatus(Constant.COMPLETED_PLANSTATUS);
		}
		if (producedQuantity != null) {
			if (producedQuantity > schedulingQuantity) {
				throw new ParameterException("已完成数量不能超过排产数量");
			}
			schedulingPlan.setProducedQuantity(producedQuantity).setSchedulingQuantity(schedulingQuantity)
					.setRemainingQuantity(schedulingQuantity - producedQuantity);
			if (schedulingPlan.getRemainingQuantity()>0) {
				schedulingPlan.setIsTimeout(true);
			}
		}
		if (remainingReason != null) {
			schedulingPlan.setRemainingReason(remainingReason);
		}
		if (productionPlanningNumber != null) {
			schedulingPlan.setProductionPlanningNumber(productionPlanningNumber);
		}
		return schedulingPlan.update();
	}

	public boolean editCapacity(Integer id, String softModel, String customerModel, Integer process,
			Integer processGroup, Integer processPeopleQuantity, Integer capacity, String remark, Integer position) {
		ModelCapacity firstModelCapacity = ModelCapacity.dao.findById(id);
		if (firstModelCapacity == null) {
			throw new OperationException("机型产能不存在");
		}
		if (position != null) {
			ModelCapacity secondModelCapacity = ModelCapacity.dao.findById(position);
			if (secondModelCapacity == null) {
				throw new OperationException("机型产能不存在");
			}
			if (!secondModelCapacity.getSoftModel().equals(firstModelCapacity.getSoftModel())) {
				throw new OperationException("同一个机型才可移动位置");
			}
			if (!secondModelCapacity.getProcessGroup().equals(firstModelCapacity.getProcessGroup())) {
				throw new OperationException("同一个工序组才可移动位置");
			}
			Db.update(SQL.UPDATE_MODELCAPACITY_POSITION, firstModelCapacity.getPosition(), secondModelCapacity.getId());
			Db.update(SQL.UPDATE_MODELCAPACITY_POSITION, secondModelCapacity.getPosition(), firstModelCapacity.getId());
			return true;
		}
		if (!StrKit.isBlank(softModel) && process != null && processGroup != null) {
			ModelCapacity modelCapacity = ModelCapacity.dao.findFirst(SQL.SELECT_MODELCAPACITY_BY_MODEL_PROCESS,
					softModel, process, processGroup);
			if (modelCapacity != null) {
				throw new OperationException("机型产能已存在");
			}
		}

		ModelCapacity modelCapacity = new ModelCapacity();
		if (!StrKit.isBlank(softModel)) {
			modelCapacity.setSoftModel(softModel);
		}
		if (process != null) {
			if (Process.dao.findById(process) == null) {
				throw new OperationException("工序不存在");
			} else {
				modelCapacity.setProcess(process);
			}
		}
		if (processGroup != null) {
			if (ProcessGroup.dao.findById(processGroup) == null) {
				throw new OperationException("工序组不存在");
			} else {
				modelCapacity.setProcessGroup(processGroup);
			}
		}
		if (customerModel != null) {
			modelCapacity.setCustomerModel(customerModel);
		}

		if (processPeopleQuantity != null) {
			modelCapacity.setProcessPeopleQuantity(processPeopleQuantity);
		}
		if (capacity != null) {
			modelCapacity.setCapacity(capacity);

		}
		if (remark != null) {
			modelCapacity.setRemark(remark);
		}

		return modelCapacity.update();
	}

	public boolean checkCompleteTime(Integer schedulingQuantity, Date planStartTime, Date planCompleteTime,
			String lineChangeTime, Integer capacity) {
		Integer actualCostHours = schedulingQuantity / capacity;
		long lineChangeCostMilliseconds = (long) (Double.parseDouble(lineChangeTime) * 60 * 60 * 1000);
		long actualCostMilliseconds = actualCostHours * 60 * 60 * 1000 + lineChangeCostMilliseconds;
		long planCostMilliseconds = planCompleteTime.getTime() - planStartTime.getTime();
		if (actualCostMilliseconds > planCostMilliseconds) {
			return false;
		}
		return true;
	}

	public Page<Record> selectCapacity(Integer pageNo, Integer pageSize, String ascBy, String descBy, String softModel,
			String customerModel, Integer process) {
		StringBuilder filter = new StringBuilder();
		if (!StrKit.isBlank(softModel)) {
			filter.append(" and soft_model like '%" + softModel + " %' ");
		}
		if (!StrKit.isBlank(customerModel)) {
			filter.append(" and customer_model like '%" + customerModel + " %' ");
		}
		if (process != null) {

			if (Process.dao.findById(process) == null) {
				throw new OperationException("工序不存在");
			}
			filter.append(" and process = " + process);
		}
		SqlPara sqlPara = new SqlPara();
		sqlPara.setSql(SQL.SELECT_MODELCAPACITY + filter);
		return Db.paginate(pageNo, pageSize, sqlPara);
	}

	public Page<Record> selectPlan(String filter,Integer pageNo, Integer pageSize) {
		SqlPara sqlPara = new SqlPara();
		sqlPara.setSql(SQL.SELECT_SCHEDULINGPLAN_LINE+filter);
		return Db.paginate(pageNo, pageSize, sqlPara);
	}

}
