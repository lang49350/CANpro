package org.example.pojo.project;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.*;
import org.example.annotation.AsColumn;
import org.example.annotation.DbName;

@Data
@TableName("signal_in_group")
@DbName("message")
public class SignalInGroup {
    @TableId(value = "id", type = IdType.AUTO)
    @AsColumn("15")
    private Long id;

    @TableField(value = "name")
    @AsColumn("0")
    private String name;

    @TableField(value = "start_bit")
    @AsColumn("1")
    private String startBit;

    @TableField(value = "length")
    @AsColumn("2")
    private String length;

    @TableField(value = "byte_order")
    @AsColumn("3")
    private String byteOrder;

    @TableField(value = "is_signed")
    @AsColumn("4")
    private String isSigned;

    @TableField(value = "scale")
    @AsColumn("5")
    private String scale;

    @TableField(value = "offset")
    @AsColumn("6")
    private String offset;

    @TableField(value = "minimum")
    @AsColumn("7")
    private String minimum;

    @TableField(value = "maximum")
    @AsColumn("8")
    private String maximum;

    @TableField(value = "unit")
    @AsColumn("9")
    private String unit;

    @TableField(value = "receiver")
    @AsColumn("10")
    private String receiver;

    @TableField(value = "dbc_comment")
    @AsColumn("11")
    private String comment;

    @TableField(value = "user_comment")
    @AsColumn("12")
    private String userComment;

    // DB Id
    @TableField(value = "message_id")
    @AsColumn("13")
    private String messageId;

    @TableField(value = "dbc_metadata_id")
    @AsColumn("14")
    private String dbcMetadataId;

    @TableField(value = "groupId")
    private long groupId;

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getStartBit() {
        return startBit;
    }

    public void setStartBit(String startBit) {
        this.startBit = startBit;
    }

    public String getLength() {
        return length;
    }

    public void setLength(String length) {
        this.length = length;
    }

    public String getByteOrder() {
        return byteOrder;
    }

    public void setByteOrder(String byteOrder) {
        this.byteOrder = byteOrder;
    }

    public String getIsSigned() {
        return isSigned;
    }

    public void setIsSigned(String isSigned) {
        this.isSigned = isSigned;
    }

    public String getScale() {
        return scale;
    }

    public void setScale(String scale) {
        this.scale = scale;
    }

    public String getOffset() {
        return offset;
    }

    public void setOffset(String offset) {
        this.offset = offset;
    }

    public String getMinimum() {
        return minimum;
    }

    public void setMinimum(String minimum) {
        this.minimum = minimum;
    }

    public String getMaximum() {
        return maximum;
    }

    public void setMaximum(String maximum) {
        this.maximum = maximum;
    }

    public String getUnit() {
        return unit;
    }

    public void setUnit(String unit) {
        this.unit = unit;
    }

    public String getReceiver() {
        return receiver;
    }

    public void setReceiver(String receiver) {
        this.receiver = receiver;
    }

    public String getComment() {
        return comment;
    }

    public void setComment(String comment) {
        this.comment = comment;
    }

    public String getUserComment() {
        return userComment;
    }

    public void setUserComment(String userComment) {
        this.userComment = userComment;
    }

    public String getMessageId() {
        return messageId;
    }

    public void setMessageId(String messageId) {
        this.messageId = messageId;
    }

    public String getDbcMetadataId() {
        return dbcMetadataId;
    }

    public void setDbcMetadataId(String dbcMetadataId) {
        this.dbcMetadataId = dbcMetadataId;
    }

    public long getGroupId() {
        return groupId;
    }

    public void setGroupId(long groupId) {
        this.groupId = groupId;
    }
}
