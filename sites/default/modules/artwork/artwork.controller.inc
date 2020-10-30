<?php 


class ArtworkController extends DrupalDefaultEntityController{
    function artwork_load($aid = NULL, $vid = NULL, $reset = FALSE){
        $aids = (isset($aid)) ? [$aid] : [];
        $conditions = (isset($vid)) ? [$vid] : [];
        $artwork = $this->artwork_load_multiple($aids, $conditions, $reset);
        return $artwork ? reset($artwrok) : FALSE;
    }

    function artwork_load_multiple($aids = [], $conditions = [], $reset = FALSE){
        return entity_load()
    }
}