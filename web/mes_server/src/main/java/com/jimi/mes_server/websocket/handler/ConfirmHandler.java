package com.jimi.mes_server.websocket.handler;

import java.text.SimpleDateFormat;
import java.util.Date;

import javax.websocket.Session;

import com.jfinal.aop.Enhancer;
import com.jimi.mes_server.entity.Constant;
import com.jimi.mes_server.model.SopSite;
import com.jimi.mes_server.service.SopService;
import com.jimi.mes_server.util.ResultUtil;
import com.jimi.mes_server.websocket.container.SessionBox;

/**客户端确认处理器
 * @author   HCJ
 * @date     2019年11月5日 下午4:47:30
 */
public class ConfirmHandler {

	private SopService sopService = Enhancer.enhance(SopService.class);


	public ResultUtil confirm(Session session, String userName, String time, String result) {
		// 如果SessionBox中不存在该Session,则提示请先登录
		Integer siteId = SessionBox.getIdBySession(session);
		if (siteId == null) {
			return ResultUtil.failed(412, "请先登录");
		}
		// 如果数据库不存在该实例，则提示站点不存在
		SopSite sopSite = SopSite.dao.findById(siteId);
		if (sopSite == null) {
			return ResultUtil.failed(412, "当前站点信息不存在");
		}
		SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		Date confirmTime;
		try {
			confirmTime = dateFormat.parse(time);
		} catch (Exception e) {
			confirmTime = new Date();
		}
		String type = null;
		if (result.equals("succeed")) {
			type = Constant.OPERATOR_CONFIRMATION_SUCCEED;
		} else if (result.equals("time_out")) {
			type = Constant.OPERATOR_CONFIRMATION_TIME_OUT;
		}
		sopService.addConfirmLog(userName, confirmTime, sopSite, null, type);
		return ResultUtil.succeed("接收确认成功");
	}
}