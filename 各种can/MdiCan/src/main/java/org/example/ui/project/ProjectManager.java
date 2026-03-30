package org.example.ui.project;

import org.example.pojo.project.CProject;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class ProjectManager {
    private static ProjectManager instance;
    private Set<Map<Long,CProject>> projects;

    private ProjectManager() {
        projects = new HashSet<>();
    }

    public static synchronized ProjectManager getInstance() {
        if (instance == null) {
            instance = new ProjectManager();
        }
        return instance;
    }

    public void addProject(CProject project) {
        Map<Long,CProject> projectMap = new HashMap<>();
        projectMap.put(project.getId(),project);
        projects.add(projectMap);
    }

    public void removeProjectById(Long projectId) {
        for (Map<Long,CProject> projectMap : projects) {
            if (projectMap.containsKey(projectId)) {
                projectMap.remove(projectId);
                break;
            }
        }

    }

    public Set<Map<Long,CProject>> getProjects (){
        return projects;
    }
}
