class CreateProfiles < ActiveRecord::Migration
  def change
    create_table :profiles do |t|
      t.string :first_name
      t.string :last_name
      t.string :dob
      t.string :gender
      t.string :mobile_number
      t.string :alternate_number
      t.string :work_location
      t.string :employer
      t.integer :user_id
      t.float :latitude
      t.float :longitude

      t.timestamps
    end
  end
end
