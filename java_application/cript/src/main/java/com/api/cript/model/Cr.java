package com.api.cript.model;

import java.util.UUID;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.Table;

@Entity
@Table(name = "tb_hash")
public class Cr {

    @Id
    @GeneratedValue(strategy = GenerationType.AUTO)
    @Column(name = "id", updatable = false, nullable = false, unique = true)
    private UUID id;

    @Column(nullable = false, updatable = false)
    private String hash;

    @Column(nullable = false, updatable = false)
    private String iv;

    public Cr() {
    }

    public Cr(String hash, String iv) {
        this.hash = hash;
        this.iv = iv;
    }

    public UUID getId() {
        return id;
    }

    public String getHash() {
        return hash;
    }

    public String getIv() {
        return iv;
    }

    public void setHash(String hash) {
        this.hash = hash;
    }

    public void setIv(String iv) {
        this.iv = iv;
    }
}
