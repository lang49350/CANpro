package org.example.utils.db;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.example.pojo.data.CanMessage;

import java.io.File;
import java.io.IOException;
import java.util.List;

public class BlfParser {

    public static List<CanMessage> parseBlfJson(String jsonFilePath) {
        ObjectMapper objectMapper = new ObjectMapper();
        try {
            List<CanMessage> canMessages = objectMapper
                    .readValue(new File(jsonFilePath),
                            objectMapper.getTypeFactory().
                                    constructCollectionType(List.class, CanMessage.class));
            return canMessages;
        }catch (Exception e){
            e.printStackTrace();
            return null;
        }
    }
    public static void main(String[] args) {
        String jsonFilePath = "D:\\MdiCanMessageRecorder\\pys\\blf_json\\json_for_Logging001.blf.json";
        List<CanMessage> messages = parseBlfJson(jsonFilePath);
        System.out.println(messages);
    }
}