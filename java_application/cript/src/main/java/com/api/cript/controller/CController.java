package com.api.cript.controller;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RestController;

import com.api.cript.DTO.CrDTO;
import com.api.cript.model.Cr;

@RestController("/")
public class CController {

    @PostMapping
    public ResponseEntity<Cr> creatHash(@RequestBody CrDTO dto) {
        Cr cript = new Cr(dto.hash());

        return repo.save(cript);
    }
}
