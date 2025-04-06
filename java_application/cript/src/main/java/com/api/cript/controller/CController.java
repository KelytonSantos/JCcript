package com.api.cript.controller;

import java.net.URI;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.support.ServletUriComponentsBuilder;

import com.api.cript.DTO.CrDTO;
import com.api.cript.model.Cr;
import com.api.cript.service.CrService;

@RestController("/")
public class CController {

    @Autowired
    private CrService serv;

    @GetMapping
    public ResponseEntity<String> getW() {
        return ResponseEntity.ok().body("welcome");
    }

    @PostMapping
    public ResponseEntity<Cr> creatHash(@RequestBody CrDTO dto) {

        Cr newHash = serv.creatHash(dto);
        URI uri = ServletUriComponentsBuilder.fromCurrentRequestUri().path("/{id}")
                .buildAndExpand(newHash.getId()).toUri();
        return ResponseEntity.created(uri).body(newHash);
    }
}
