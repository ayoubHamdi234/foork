-- Ajoute les colonnes attendues par l'application pour les badges RFID et la récupération de PIN.
-- Script compatible Oracle.

-- 1) Colonnes manquantes : PIN, QUESTION, REPONSE, RFID_UID
ALTER TABLE EMPLOYES ADD (
    PIN        VARCHAR2(10 CHAR),
    QUESTION   VARCHAR2(255 CHAR),
    REPONSE    VARCHAR2(255 CHAR),
    RFID_UID   VARCHAR2(32 CHAR)
);

-- 2) Normalise les UID déjà présents pour éviter les erreurs de casse/espaces
UPDATE EMPLOYES SET RFID_UID = UPPER(TRIM(RFID_UID));

-- 3) Contrainte : PIN numérique (4 à 10 chiffres)
ALTER TABLE EMPLOYES ADD CONSTRAINT EMP_PIN_NUMERIC CHECK (REGEXP_LIKE(PIN, '^\\d{4,10}$'));

-- 4) Index pour accélérer la recherche par badge
CREATE INDEX EMP_RFID_UID_IDX ON EMPLOYES (RFID_UID);
