package org.example.ui.project.chart.group;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import lombok.Data;
import org.example.mapper.SignalGroupMapper;
import org.example.mapper.SignalInGroupMapper;
import org.example.pojo.dbc.DbcSignal;
import org.example.pojo.project.SignalGroup;
import org.example.pojo.project.SignalInGroup;
import org.example.utils.db.DatabaseManager;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;

@Data
public class SignalManager {
    private static SignalManager instance;
    private DatabaseManager dbManager;

    private SignalManager() {
        dbManager = new DatabaseManager();
        instance = this;
    }

    public static synchronized SignalManager getInstance() {
        if (instance == null) {
            instance = new SignalManager();
        }
        return instance;
    }

    // 外部调用方法：将信号添加到指定组或新组

    // 新增：获取所有信号组及其包含的信号
    public List<SignalGroupWithSignals> getAllGroupsWithSignals() {
        List<SignalGroupWithSignals> result = new ArrayList<>();

        // 获取所有信号组
        List<SignalGroup> groups = dbManager.execute(SignalGroup.class,session -> {
            SignalGroupMapper mapper = session.getMapper(SignalGroupMapper.class);
            return mapper.selectList(null);
        });

        for (SignalGroup group : groups) {
            // 获取每个组的信号
            List<SignalInGroup> signals = dbManager.execute(SignalGroup.class,session -> {
                SignalInGroupMapper mapper = session.getMapper(SignalInGroupMapper.class);
                QueryWrapper<SignalInGroup> wrapper = new QueryWrapper<>();
                wrapper.lambda().eq(SignalInGroup::getGroupId, group.getId());
                return mapper.selectList(wrapper);
            });
            String creationTime = group.getCreationTime();
            // 转字符串
            // 创建带信号的信号组对象
            SignalGroupWithSignals groupWithSignals = new SignalGroupWithSignals(
                    group.getId(),
                    group.getName(),
                    group.getCreator(),
                    creationTime,
                    group.getRemark(),
                    signals
            );

            result.add(groupWithSignals);
        }

        return result;
    }


    // 新增内部类：带信号的信号组
    @Data
    public static class SignalGroupWithSignals {
        private int id;
        private String name;
        private String creator;
        private String createTime;
        private String remark;
        private List<SignalInGroup> signals;

        public SignalGroupWithSignals(int id, String name, String creator, String createTime, String remark, List<SignalInGroup> signals) {
            this.id = id;
            this.name = name;
            this.creator = creator;
            this.createTime = createTime;
            this.remark = remark;
            this.signals = signals;
        }
    }
}