package org.example.service;

import com.baomidou.mybatisplus.core.conditions.Wrapper;
import com.baomidou.mybatisplus.extension.service.IService;
import org.example.pojo.data.CanMessage;

import java.util.Collection;
import java.util.List;

public interface CanMessageService extends IService<CanMessage> {

    boolean saveDynamic(CanMessage entity,String suffix);

    boolean saveBatchDynamic(Collection<CanMessage> entityList, int batchSize,String suffix);

    boolean saveBatchDynamic(Collection<CanMessage> entityList,String suffix);


    List<CanMessage> listDynamic(Wrapper<CanMessage> queryWrapper,String suffix);


    List<CanMessage> listDynamic(String suffix);
}
