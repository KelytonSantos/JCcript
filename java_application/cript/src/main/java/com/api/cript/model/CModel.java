package com.api.cript.model;

import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.Table;

@Entity
@Table(name = "tb_hash")
public class CModel {


    @Id
    @GeneratedValue(strategy = GenerationType.AUTO)
    private String id;

    private String hash;



    void 
}
