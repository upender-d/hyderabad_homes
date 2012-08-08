class Administrators::OwnershipTypesController < ApplicationController

  def index
    if params[:search]
      @ownership_types = OwnershipType.search_properties(params[:search]).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC")
    else
      @ownership_types = OwnershipType.paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC")
    end
  end

  def new
    @ownership_type = OwnershipType.new
  end

  def create
    @ownership_type = OwnershipType.new(params[:ownership_type])
    #@ownership_type.user_id = current_user.id
    respond_to do |format|
      if  @ownership_type.save
        format.html { redirect_to ([:administrators,@ownership_type]) ,:notice => "Property Saved Successfully." }
      else
        format.html { render :action => "new" }
        format.xml  { render :xml => @ownership_type.errors, :status => :unprocessable_entity }
      end
    end
  end

  def edit
    @ownership_type = OwnershipType.find(params[:id])
  end

  def update
    @ownership_type = Property.find(params[:id])
    #@ownership_type.user_id = current_user.id
    if  @ownership_type.update_attributes(params[:ownership_type])
      redirect_to([:administrators,@ownership_type])
      flash[:notice] = "Property Updated Successfully."
    else
      render :action => "edit"
    end
  end

  def show
    @ownership_type = OwnershipType.find(params[:id])
  end

  def destroy
    @ownership_type = Property.find(params[:id])
    @ownership_type.destroy
    redirect_to administrators_ownership_types_path
  end

end
