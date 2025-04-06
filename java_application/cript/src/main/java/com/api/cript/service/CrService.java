package com.api.cript.service;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import com.api.cript.DTO.CrDTO;
import com.api.cript.model.Cr;
import com.api.cript.repo.CrRepo;

@Service
public class CrService {

    @Autowired
    private CrRepo repo;

    public Cr creatHash(CrDTO dto) {
        Cr newHash = new Cr(dto.hash());

        return repo.save(newHash);
    }

}
