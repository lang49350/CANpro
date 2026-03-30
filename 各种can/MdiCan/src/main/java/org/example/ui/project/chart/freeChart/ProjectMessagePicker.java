package org.example.ui.project.chart.freeChart;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import lombok.Data;
import org.example.mapper.CanMessageMapper;
import org.example.pojo.data.CanMessage;
import org.example.utils.db.DatabaseManager;
import org.example.utils.db.TableNameContextHolder;
import org.springframework.stereotype.Component;

import java.util.List;

// 根据项目代码从项目中提取消息
@Data
@Component
public class ProjectMessagePicker {
    private DatabaseManager databaseManager;

    public ProjectMessagePicker() {
        this.databaseManager = new DatabaseManager();
    }

    public List<CanMessage> pickMessages(Long projectId, String CanMessageId, double maxTimePoint) {
        // 从数据库中获取项目下的所有消息
        return databaseManager.execute(CanMessage.class, session -> {
            CanMessageMapper mapper = session.getMapper(CanMessageMapper.class);
            QueryWrapper<CanMessage> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(CanMessage::getProjectId, projectId)
                    .eq(CanMessage::getCanId, CanMessageId)
                    .apply("CAST(time as DOUBLE)>{0}", maxTimePoint)
                    .orderByAsc(CanMessage::getIndexCounter);
            try{
                TableNameContextHolder.setSuffix("can_message","_p_" + projectId);
                return mapper.selectList(wrapper);
            }catch (Exception e){
                TableNameContextHolder.clearSuffix("can_message");
                return null;
            }
        });
    }

}
