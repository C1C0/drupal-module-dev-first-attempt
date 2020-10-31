<?php


class ArtworkController extends DrupalDefaultEntityController
{

    //load() is inherited from DrupalDefaultEntityController

    public function create($type = '')
    {
        return (object) [
            'aid' => '',
            'type' => $type,
            'title' => '',
        ];
    }

    public function save($artwork)
    {
        //starts session with db
        //can use multiple queries
        $transation = db_transaction();

        try {
            global $user;

            //determine if we will be inserting a new artwork
            $artwork->is_new = empty($artwork->aid);

            //set timestamp fields
            if (empty($artwork->created)) {
                $artwork->created = REQUEST_TIME;
            }
            $artwork->changed = REQUEST_TIME;

            $artwork->revision_timestamp = REQUEST_TIME;
            $update_artwork = true;

            //give modules the opportunity to prepare field data for saving
            field_attach_presave('artwork', $artwork);

            if (!$artwork->is_new && !empty($artwork->revision) && $artwork->vid) {
                $artwork->old_vid = $artwork->vid;
                unset($artwork->vid);
            }

            //if this is a new artwork
            if ($artwork->is_new) {
                //save the new artwork.
                drupal_write_record('artwork', $artwork);

                //save the initail revision.
                $this->saveRevision($artwork, $user->uid);

                $op = 'insert';
            } else {
                //save the updated artwork
                drupal_write_record('artwork', $artwork, 'aid');

                if (!empty($artwork->revision)) {
                    $this->saveRevision($artwork, $user->uid);
                } else {
                    $this->saveRevision($artwork, $user->uid, TRUE);
                    $update_artwork = FALSE;
                }

                $op = 'update';
            }

            //if the revision id is new or updated, save it to the artwork
            if ($update_artwork) {
                db_update('artwork')
                    ->fields(['vid' => $artwork->vid])
                    ->condition('aid', $artwork->aid)
                    ->execute();
            }

            //save fields
            $function = 'field_attach_' . $op;
            $function('artwork', $artwork);

            //let other modules interact with any entity whn they are created or
            //updated
            module_invoke_all('entity_' . $op, $artwork, 'artwork');

            //clear internal properties.

            unset($artwork->is_new);

            //ignore slave server temporarily to give time for the saved
            //order to be propagated to the slave.
            //only master server will update it so other users might not see it
            //immediately
            db_ignore_slave();

            return $artwork;
        } catch (Exception $e) {
            $transation->rollback();
            watchdog_exception('artwork', $e, NULL, WATCHDOG_ERROR);
            return false;
        }
    }

    function saveRevision($artwork, $uid, $update = FALSE){
        //hold on to the artwrok's original creator_uid but swap
        //in the revision's creator_uid for the momentary write
        $temp_uid= $artwork->uid;
        $artwork->uid = $uid;

        if($update){
            drupal_write_record('artwork_revision', $artwork, 'vid');
        }else{
            drupal_write_record('artwork_revision', $artwrok);
        }

        //reset the order's creator_uid to the original value
        $artwork->uid = $temp_uid;

    }

    public function delete($aids){
        if(!empty($aids)){
            $artworks = $this->load($aids, []);
            $transaction = db_transaction();

            try{
                db_delete('artwork')
                    ->condition('aid', $aids, 'IN')
                    ->execute();

                db_delete('artwork_revision')
                    ->condition('aid', $aids, 'IN')
                    ->execute();

                foreach($artworks as $artwork_id => $artwork){
                    field_attach_delete('artwork', $artwork);
                }

                db_ignore_slave();
            }catch(Exception $e){
                $transaction->rollback();
                watchdog_exception('artwork', $e, NULL, WATCHDOG_ERROR);
                return FALSE;
            }

            module_invoke_all('entity_delete', $artwork, 'artwork');

            //clear the page and block and artwork caches
            cache_clear_all();
            $this->resetCache();
        }
        return TRUE;
    }
}
